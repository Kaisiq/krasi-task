#include "common.h"

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

    printf("%s%s[Process 3 - PID: %d] Started. Delay: %d ms. Reading floats "
           "-> Message Queue (%s)%s\n",
           fg_code, bg_code, my_pid, delay_ms, MSGQ_NAME, COLOR_RESET);
    fflush(stdout);

    mqd_t mq;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(mq_msg_float);
    attr.mq_curmsgs = 0;

    mq = mq_open(MSGQ_NAME, O_WRONLY | O_CREAT, 0666, &attr);
    if (mq == (mqd_t)-1) {
      perror("Process 3: Failed to open message queue");
      return EXIT_FAILURE;
    }

    double input_float;
    mq_msg_float msg;
    msg.mtype = 1;
    msg.source_pid = my_pid;

    printf("%s%s[Process 3 - PID: %d] Message Queue opened. Enter floats "
           "(Ctrl+D or signal to stop):\n%s",
           fg_code, bg_code, my_pid, COLOR_RESET);
    fflush(stdout);

    while (!terminate_flag) {
      printf("%s%sP3 Input> %s", fg_code, bg_code, COLOR_RESET);
      fflush(stdout);

      sleep(1);
      if (kbhit()) {
        if (scanf("%lf", &input_float) == 1) {
          while (getchar() != '\n')
            ;

          msg.value = input_float;

          if (mq_send(mq, (const char *)&msg, sizeof(msg), 0) == -1) {
            perror("Process 3: Failed to send message");
            terminate_flag = 1;
          }

        } else {
          if (feof(stdin)) {
            printf("\n%s%s[Process 3 - PID: %d] EOF detected on input. "
                   "Terminating.%s\n",
                   fg_code, bg_code, my_pid, COLOR_RESET);
          } else {
            printf("\n%s%s[Process 3 - PID: %d] Invalid input. Terminating.%s\n",
                   fg_code, bg_code, my_pid, COLOR_RESET);
            while (getchar() != '\n' && !feof(stdin) && !ferror(stdin))
              ;
          }
          terminate_flag = 1;
          break;
        }
      }

      if (!terminate_flag && delay_ms > 0) {
        usleep(delay_ms * 1000);
      }
    }

    printf("%s%s[Process 3 - PID: %d] Terminating and closing Message Queue.%s\n",
           fg_code, bg_code, my_pid, COLOR_RESET);

    if (mq != (mqd_t)-1) {
      mq_close(mq);
      mq_unlink(MSGQ_NAME);
    }

    return EXIT_SUCCESS;
  }
