#include "common.h"
#include <fcntl.h>      // <<<--- ДОБАВЕНО: За open()
#include <sys/stat.h>   // <<<--- ДОБАВЕНО: За open() flags
#include <time.h>
#include <errno.h>

// #include <mqueue.h> // <<<--- ПРЕМАХНАТО

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

    signal(SIGTERM, handle_sigterm);

    // <<<--- ПРОМЯНА: Актуализиран текст и използване на FIFO_PATH_P3
    printf("%s%s[Process 3 - PID: %d] Started. Delay: %d ms. Reading floats -> FIFO (%s)%s\n",
           fg_code, bg_code, my_pid, delay_ms, FIFO_PATH_P3, COLOR_RESET);
    fflush(stdout);

    int fifo_fd = -1; // <<<--- ПРОМЯНА: Променлива за FIFO дескриптор
    int attempts = 0;
    const int max_attempts = 5;

    // <<<--- ПРОМЯНА: Логика за отваряне на FIFO ---
    while (attempts < max_attempts && !terminate_flag) {
        fifo_fd = open(FIFO_PATH_P3, O_WRONLY); // Отвори новия FIFO за писане
        if (fifo_fd >= 0) {
            break; // Success
        }
        if (errno == ENOENT) { // No such file or directory
            attempts++;
            printf("%s%s[Process 3 - PID: %d] FIFO P3 not found (attempt %d/%d). Waiting for P5...%s\n",
                   fg_code, bg_code, my_pid, attempts, max_attempts, COLOR_RESET);
            sleep(1); // Wait 1 second before retrying
        } else {
            perror("Process 3: Failed to open FIFO P3 for writing");
            printf("%s%s[Process 3 - PID: %d] Terminating due to FIFO P3 open error.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
            return EXIT_FAILURE; // Fail on other errors
        }
    }

    if (fifo_fd < 0) {
        fprintf(stderr, "%s%s[Process 3 - PID: %d] Failed to open FIFO P3 after %d attempts. Terminating.%s\n",
                fg_code, bg_code, my_pid, max_attempts, COLOR_RESET);
        return EXIT_FAILURE;
    }
    // --- Край на промяната ---

    double input_float;
    fifo_msg_float msg; // <<<--- ПРОМЯНА: Използване на новата структура
    msg.source_pid = my_pid;

    printf("%s%s[Process 3 - PID: %d] FIFO P3 opened. Enter floats (Ctrl+D or signal to stop):\n%s", fg_code, bg_code, my_pid, COLOR_RESET);
    fflush(stdout);

    while (!terminate_flag) {
        printf("%s%sP3 Input> %s", fg_code, bg_code, COLOR_RESET);
        fflush(stdout);

        if (scanf("%lf", &input_float) == 1) {
             while (getchar() != '\n'); // Consume trailing newline

            msg.value = input_float;

            // <<<--- ПРОМЯНА: Изпращане чрез write() към FIFO ---
            ssize_t bytes_written = write(fifo_fd, &msg, sizeof(msg));
            if (bytes_written < 0) {
                if (errno == EPIPE) {
                    printf("\n%s%s[Process 3 - PID: %d] Detected P5 closed the FIFO P3. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
                    terminate_flag = 1; // Stop loop
                } else {
                    perror("Process 3: Failed to write to FIFO P3");
                    terminate_flag = 1; // Terminate on other write errors
                }
            } else if (bytes_written < sizeof(msg)) {
                 fprintf(stderr, "%s%s[Process 3 - PID: %d] Warning: Partial write to FIFO P3.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
            } else {
                 // printf("%s%s[Process 3 - PID: %d] Sent: %f%s\n", fg_code, bg_code, my_pid, input_float, COLOR_RESET); // Optional: Verbose output
                 fflush(stdout);
            }
            // --- Край на промяната ---

        } else {
            if (feof(stdin)) {
                 printf("\n%s%s[Process 3 - PID: %d] EOF detected on input. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
            } else {
                 printf("\n%s%s[Process 3 - PID: %d] Invalid input. Terminating.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
                 while (getchar() != '\n' && !feof(stdin) && !ferror(stdin));
            }
             terminate_flag = 1;
             break;
        }

        if (!terminate_flag && delay_ms > 0) {
            usleep(delay_ms * 1000);
        }
    }

    printf("%s%s[Process 3 - PID: %d] Terminating and closing FIFO P3.%s\n", fg_code, bg_code, my_pid, COLOR_RESET);
    if (fifo_fd >= 0) { // <<<--- ПРОМЯНА: Затваряне на FIFO дескриптора
        close(fifo_fd);
    }

    return EXIT_SUCCESS;
}
