#include "common.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

pid_t pids[6] = {
    0}; // pids[0] unused, pids[1] unused, pids[2-4] for P2-P4, pids[5] for P5
int running_status[6] = {0}; // 0=stopped, 1=running

char p5_log_filename[256] = "process5_log.txt";

void ensure_p5_started() {
  if (pids[5] == 0) {
    printf("Starting Process 5 (Logging Process)...\n");

    pids[5] = fork();
    if (pids[5] < 0) {
      perror("Failed to fork Process 5");
      pids[5] = 0; // Reset PID
    } else if (pids[5] == 0) {
      char *args[] = {"./process5", p5_log_filename, NULL};
      execvp("./process5", args);
      perror("Failed to exec Process 5");
      exit(EXIT_FAILURE);
    } else {
      printf("Process 5 started with PID: %d\n", pids[5]);
      running_status[5] = 1;
      printf("Waiting for Process 5 to initialize...\n");
      sleep(2);
    }
  }
}

void try_stop_p5() {
  if (pids[5] != 0 && running_status[2] == 0 && running_status[3] == 0 &&
      running_status[4] == 0) {
    printf("Stopping Process 5 (PID: %d)...\n", pids[5]);
    if (kill(pids[5], SIGTERM) == 0) {
      int status;
      waitpid(pids[5], &status, 0);
      printf("Process 5 terminated.\n");
    } else {
      if (errno == ESRCH) {
        printf("Process 5 already terminated.\n");
      } else {
        perror("Failed to send SIGTERM to Process 5");
      }
    }
    pids[5] = 0;
    running_status[5] = 0;
  }
}

void start_child(int process_num) {
  if (process_num < 2 || process_num > 4)
    return;
  if (pids[process_num] != 0) {
    printf("Process %d is already running (PID: %d).\n", process_num,
           pids[process_num]);
    return;
  }

  int fg_color, bg_color, delay_ms;
  printf("--- Configure Process %d ---\n", process_num);
  printf("Enter text color (0-7, e.g., 0=Black, 1=Red, 2=Green, 7=White): ");
  if (scanf("%d", &fg_color) != 1) {
    printf("Invalid input.\n");
    while (getchar() != '\n')
      ;
    return;
  }
  printf("Enter background color (0-7): ");
  if (scanf("%d", &bg_color) != 1) {
    printf("Invalid input.\n");
    while (getchar() != '\n')
      ;
    return;
  }
  printf("Enter pause duration between cycles (milliseconds): ");
  if (scanf("%d", &delay_ms) != 1) {
    printf("Invalid input.\n");
    while (getchar() != '\n')
      ;
    return;
  }
  while (getchar() != '\n')
    ;

  ensure_p5_started();

  if (pids[5] == 0 || kill(pids[5], 0) != 0) {
    printf("Cannot start Process %d because Process 5 is not running or failed "
           "to start properly.\n",
           process_num);
    if (pids[5] != 0 && errno == ESRCH) {
      running_status[5] = 0;
      pids[5] = 0;
    }
    return;
  }

  printf("Starting Process %d...\n", process_num);
  pids[process_num] = fork();

  if (pids[process_num] < 0) {
    perror("Failed to fork child process");
    pids[process_num] = 0;
  } else if (pids[process_num] == 0) {
    char fg_str[5], bg_str[5], delay_str[10];
    snprintf(fg_str, sizeof(fg_str), "%d", fg_color);
    snprintf(bg_str, sizeof(bg_str), "%d", bg_color);
    snprintf(delay_str, sizeof(delay_str), "%d", delay_ms);

    char executable_name[20];
    snprintf(executable_name, sizeof(executable_name), "./process%d",
             process_num);

    char *args[] = {executable_name, fg_str, bg_str, delay_str, NULL};
    execvp(executable_name, args);
    perror("Failed to exec child process");
    exit(EXIT_FAILURE);
  } else {
    printf("Process %d started with PID: %d\n", process_num, pids[process_num]);
    running_status[process_num] = 1;
  }
  usleep(delay_ms * 1000);
}

void stop_child(int process_num) {
  if (process_num < 2 || process_num > 4)
    return;
  if (pids[process_num] == 0) {
    printf("Process %d is not running.\n", process_num);
    return;
  }

  printf("Stopping Process %d (PID: %d)...\n", process_num, pids[process_num]);
  if (kill(pids[process_num], SIGTERM) == 0) {
    int status;
    waitpid(pids[process_num], &status, 0);
    printf("Process %d terminated.\n", process_num);
  } else {
    if (errno == ESRCH) {
      printf("Process %d already terminated.\n", process_num);
    } else {
      perror("Failed to send SIGTERM to child process");
      printf("Attempting force kill (SIGKILL)...\n");
      kill(pids[process_num], SIGKILL);
      waitpid(pids[process_num], NULL, 0);
    }
  }

  pids[process_num] = 0;
  running_status[process_num] = 0;

  try_stop_p5();
}

void display_menu() {
  printf("\n--- Main Menu ---\n");
  printf("1. Start Process 2 (Integers -> FIFO)\t\t[%s]\n",
         running_status[2] ? "Running" : "Stopped");
  printf("2. Start Process 3 (Floats -> MSGQueue)\t\t[%s]\n",
         running_status[3] ? "Running" : "Stopped");
  printf("3. Start Process 4 (Strings -> Socket)\t\t[%s]\n",
         running_status[4] ? "Running" : "Stopped");
  printf("--------------------\n");
  printf("4. Stop Process 2\n");
  printf("5. Stop Process 3\n");
  printf("6. Stop Process 4\n");
  printf("--------------------\n");
  printf("7. Exit Program\n");
  if (pids[5] != 0 && kill(pids[5], 0) == -1 && errno == ESRCH) {
    running_status[5] = 0;
    pids[5] = 0;
  }
  printf("Process 5 (Logger) Status:\t\t\t[%s]\n",
         running_status[5] ? "Running" : "Stopped");
  printf("Enter your choice: ");
}

int main() {
  int choice;

  do {
    display_menu();
    sleep(1);
    if (!kbhit()) {
      sleep(1);
      continue;
    }

    if (scanf("%d", &choice) != 1) {
      printf("Invalid input. Please enter a number.\n");
      while (getchar() != '\n')
        ;
      choice = 0;
      usleep(2000 * 1000);
      continue;
    }
    while (getchar() != '\n')
      ;

    switch (choice) {
    case 1:
      start_child(2);
      break;
    case 2:
      start_child(3);
      break;
    case 3:
      start_child(4);
      break;
    case 4:
      stop_child(2);
      break;
    case 5:
      stop_child(3);
      break;
    case 6:
      stop_child(4);
      break;
    case 7:
      if (running_status[2] != 0 || running_status[3] != 0 ||
          running_status[4] != 0) {
        printf("Error: Cannot exit while Process 2, 3, or 4 are running.\n");
        printf("Please stop all child processes first.\n");
      } else {
        if (running_status[5] != 0) {
          printf("Process 5 is still running. Stopping it now...\n");
          try_stop_p5();
        }
        if (running_status[5] == 0) {
          printf("Exiting program.\n");
        } else {
          printf("Error: Failed to stop Process 5. Cannot exit cleanly.\n");
          choice = 0;
        }
      }

      break;
    default:
      printf("Invalid choice. Please try again.\n");
    }
  } while (choice != 7 || running_status[2] != 0 || running_status[3] != 0 ||
           running_status[4] != 0 || running_status[5] != 0);

  printf("Main program finished.\n");
  return 0;
}
