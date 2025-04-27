#include "common.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

volatile sig_atomic_t terminate_flag = 0;

void handle_sigterm(int sig) { terminate_flag = 1; }

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <fg_color> <bg_color> <delay_ms>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int fg_color = atoi(argv[1]);
  int bg_color = atoi(argv[2]);
  int delay_ms = atoi(argv[3]);
  pid_t my_pid = getpid();

  const char *fg_code = get_color_code(fg_color, 0);
  const char *bg_code = get_color_code(bg_color, 1);

  signal(SIGTERM, handle_sigterm);

  printf("%s%s[Process 4 - PID: %d] Started. Delay: %d ms. Reading strings -> "
         "Socket (%s)%s\n",
         fg_code, bg_code, my_pid, delay_ms, SOCKET_PATH, COLOR_RESET);
  fflush(stdout);

  int sock_fd = -1;
  struct sockaddr_un server_addr;

  // Create socket
  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("Process 4: Failed to create socket");
    printf("%s%s[Process 4 - PID: %d] Terminating due to Socket error.%s\n",
           fg_code, bg_code, my_pid, COLOR_RESET);
    return EXIT_FAILURE;
  }

  // Prepare server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

  // Connect to server (P5) - Retry briefly if P5 isn't ready yet
  int connection_attempts = 5;
  while (connect(sock_fd, (struct sockaddr *)&server_addr,
                 sizeof(server_addr)) < 0) {
    if (errno == ENOENT ||
        errno ==
            ECONNREFUSED) { // Socket file doesn't exist or server not listening
      connection_attempts--;
      if (connection_attempts <= 0) {
        perror("Process 4: Failed to connect to socket after retries");
        printf("%s%s[Process 4 - PID: %d] Terminating - P5 not available?%s\n",
               fg_code, bg_code, my_pid, COLOR_RESET);
        close(sock_fd);
        return EXIT_FAILURE;
      }
      printf("%s%s[Process 4 - PID: %d] P5 socket not ready, retrying...%s\n",
             fg_code, bg_code, my_pid, COLOR_RESET);
      sleep(1); // Wait 1 second before retrying
    } else {
      perror("Process 4: Error connecting to socket");
      close(sock_fd);
      printf("%s%s[Process 4 - PID: %d] Terminating due to Socket connection "
             "error.%s\n",
             fg_code, bg_code, my_pid, COLOR_RESET);
      return EXIT_FAILURE;
    }
  }

  printf("%s%s[Process 4 - PID: %d] Connected to P5 socket.%s\n", fg_code,
         bg_code, my_pid, COLOR_RESET);
  fflush(stdout);

  char input_string[SOCK_MSG_MAX_LEN];
  sock_msg_string msg;
  msg.source_pid = my_pid;

  printf(
      "%s%s[Process 4 - PID: %d] Enter strings (Ctrl+D or signal to stop):\n%s",
      fg_code, bg_code, my_pid, COLOR_RESET);
  fflush(stdout);

  while (!terminate_flag) {
    printf("%s%sP4 Input> %s", fg_code, bg_code, COLOR_RESET);
    fflush(stdout);
    sleep(1);

    if (kbhit()) {

      if (fgets(input_string, sizeof(input_string), stdin) != NULL) {
        input_string[strcspn(input_string, "\n")] =
            0; // Remove trailing newline

        if (strlen(input_string) == 0 && !terminate_flag) {
          continue;
        }

        strncpy(msg.value, input_string, SOCK_MSG_MAX_LEN - 1);
        msg.value[SOCK_MSG_MAX_LEN - 1] = '\0'; // Ensure null termination

        ssize_t bytes_sent = send(sock_fd, &msg, sizeof(msg), 0);
        if (bytes_sent < 0) {
          if (errno == EPIPE) {
            printf("\n%s%s[Process 4 - PID: %d] Detected P5 closed the socket "
                   "connection. Terminating.%s\n",
                   fg_code, bg_code, my_pid, COLOR_RESET);
            terminate_flag = 1;
          } else {
            perror("Process 4: Failed to send data");
            terminate_flag = 1;
          }
        } else if (bytes_sent < sizeof(msg)) {
          fprintf(
              stderr,
              "%s%s[Process 4 - PID: %d] Warning: Partial send on socket.%s\n",
              fg_code, bg_code, my_pid, COLOR_RESET);
        } else {
          printf("%s%s[Process 4 - PID: %d] Sent: \"%s\"%s\n", fg_code, bg_code,
                 my_pid, msg.value, COLOR_RESET);
          fflush(stdout);
        }
      } else {
        if (feof(stdin)) {
          printf("\n%s%s[Process 4 - PID: %d] EOF detected on input. "
                 "Terminating.%s\n",
                 fg_code, bg_code, my_pid, COLOR_RESET);
        } else {
          printf("\n%s%s[Process 4 - PID: %d] Input error. Terminating.%s\n",
                 fg_code, bg_code, my_pid, COLOR_RESET);
        }
        terminate_flag = 1;
        break;
      }
    }

    if (!terminate_flag && delay_ms > 0) {
      usleep(delay_ms * 1000);
    }
  }

  printf("%s%s[Process 4 - PID: %d] Terminating and closing socket.%s\n",
         fg_code, bg_code, my_pid, COLOR_RESET);
  if (sock_fd >= 0) {
    close(sock_fd);
  }

  return EXIT_SUCCESS;
}
