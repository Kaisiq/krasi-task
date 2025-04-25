#include "common.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>   // For usleep
#include <errno.h>

volatile sig_atomic_t terminate_flag = 0;

void handle_sigterm(int sig) {
    terminate_flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <fg_color> <bg_color> <delay_ms>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fg_color = atoi(argv[1]);
    int bg_color = atoi(argv[2]);
    int delay_ms = atoi(argv[3]);
    pid_t my_pid = getpid();

    const char* fg_code = get_color_code(fg_color, 0);
    const char* bg_code = get_color_code(bg_color, 1);

    // Setup signal handler
    signal(SIGTERM, handle_sigterm);

    printf("%s%s[Process 2 - PID: %d] Started. Delay: %d ms. Reading integers -> FIFO (%s)%s\n",
           fg_code, bg_code, my_pid, delay_ms, FIFO_PATH_P2, COLOR_RESET);
    fflush(stdout);

    int fifo_fd = -1;
    fifo_fd = open(FIFO_PATH_P2, O_WRONLY);
    if (fifo_fd < 0) {
        printf("%s%s[Process 2 - PID: %d] Terminating due to FIFO error.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
        return EXIT_FAILURE;
    }

    int input_int;
    fifo_msg_int msg;
    msg.source_pid = my_pid;

    printf("%s%s[Process 2 - PID: %d] Enter integers (Ctrl+D or signal to stop):\n%s", fg_code, bg_code, my_pid, COLOR_RESET);
    fflush(stdout);

    while (!terminate_flag) {
        printf("%s%sP2 Input> %s", fg_code, bg_code, COLOR_RESET);
        fflush(stdout);

        // Use select or poll for non-blocking input? Simpler: blocking scanf
        if (scanf("%d", &input_int) == 1) {
             while (getchar() != '\n'); // Consume trailing newline

            msg.value = input_int;
            ssize_t bytes_written = write(fifo_fd, &msg, sizeof(msg));
            if (bytes_written < 0) {
                if (errno == EPIPE) {
                    printf("\n%s%s[Process 2 - PID: %d] Detected P5 closed the FIFO. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
                    terminate_flag = 1; // Stop loop
                } else {
                    perror("Process 2: Failed to write to FIFO");
                    // Decide whether to terminate or continue
                }
            } else if (bytes_written < sizeof(msg)) {
                 fprintf(stderr, "%s%s[Process 2 - PID: %d] Warning: Partial write to FIFO.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
            } else {
                 printf("%s%s[Process 2 - PID: %d] Sent: %d%s\n", fg_code, bg_code, my_pid, input_int, COLOR_RESET);
                 fflush(stdout);
            }
            if (!terminate_flag && delay_ms > 0) {
                usleep(delay_ms * 1000);
                waitForNewline();
            }
        } else {
            // scanf failed (e.g., EOF or invalid input)
            if (feof(stdin)) {
                printf("\n%s%s[Process 2 - PID: %d] EOF detected on input. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
            } else {
                 printf("\n%s%s[Process 2 - PID: %d] Invalid input. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
                 // Clear the invalid input
                 while (getchar() != '\n' && !feof(stdin) && !ferror(stdin));
            }
             terminate_flag = 1; // Stop loop on input error/EOF
             break; // Exit loop immediately
        }
    }

    printf("%s%s[Process 2 - PID: %d] Terminating and closing FIFO.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
    if (fifo_fd >= 0) {
        close(fifo_fd);
    }

    return EXIT_SUCCESS;
}
