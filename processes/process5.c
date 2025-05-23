#include <string.h>
#define _POSIX_C_SOURCE 200809L // Или може да опитате с 199309L
#include "common.h"
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

volatile sig_atomic_t terminate_flag = 0;

// FDs for IPC
int fifo_p2_fd = -1;  // <<<--- ПРОМЯНА: Преименуван за яснота
mqd_t mq = (mqd_t)-1; // Message queue descriptor
int listen_sock_fd = -1;
int client_sock_fd = -1;
FILE *log_file = NULL;

void handle_sigterm(int sig) {
  terminate_flag = 1;
  printf("\n[Process 5] SIGTERM/SIGINT received. Shutting down...\n");
  fflush(stdout);
}

void cleanup_ipc() {
  printf("[Process 5] Cleaning up IPC resources...\n");
  fflush(stdout);
  if (log_file) {
    fclose(log_file);
    log_file = NULL;
  }
  if (fifo_p2_fd >= 0) {
    close(fifo_p2_fd);
    fifo_p2_fd = -1;
  }
  if (mq != (mqd_t)-1) {
    mq_close(mq);
    mq_unlink(MSGQ_NAME);
    mq = (mqd_t)-1;
  }
  // if (mq != (mqd_t)-1) { mq_close(mq); mq = (mqd_t)-1; } // <<<--- ПРЕМАХНАТО
  if (client_sock_fd >= 0) {
    close(client_sock_fd);
    client_sock_fd = -1;
  }
  if (listen_sock_fd >= 0) {
    close(listen_sock_fd);
    listen_sock_fd = -1;
  }

  if (unlink(FIFO_PATH_P2) < 0 && errno != ENOENT)
    perror("  unlink fifo_p2 failed");
  if (mq_unlink(MSGQ_NAME) < 0 && errno != ENOENT)
    perror("  mq_unlink failed");
  if (unlink(SOCKET_PATH) < 0 && errno != ENOENT)
    perror("  unlink socket failed");
  printf("[Process 5] Cleanup attempts complete.\n");
  fflush(stdout);
}

int openP2File() {
  if (mkfifo(FIFO_PATH_P2, FIFO_PERMS) < 0 &&
      errno != EEXIST) { // <<<--- ПРОМЯНА: Права и име
    perror("  ERROR: Failed to create FIFO P2");
    return 0;
  }
  fifo_p2_fd =
      open(FIFO_PATH_P2,
           O_RDONLY | O_NONBLOCK); // <<<--- ПРОМЯНА: Име на променлива и път
  if (fifo_p2_fd < 0) {
    perror("  ERROR: Failed to open FIFO P2 for reading");
    return 0;
  }
  return 1;
}

int openP3File() {
  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(mq_msg_float);
  attr.mq_curmsgs = 0;

  mq = mq_open(MSGQ_NAME, O_RDONLY | O_CREAT | O_NONBLOCK, 0666, &attr);
  if (mq == (mqd_t)-1) {
    perror("  ERROR: Failed to open message queue");
    return 0;
  }
  return 1;
}

int openP4File() {
  struct sockaddr_un local_addr;
  listen_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_sock_fd < 0) {
    perror("  ERROR: Failed to create listening socket");
    return 0;
  } else {
    unlink(SOCKET_PATH);
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sun_family = AF_UNIX;
    strncpy(local_addr.sun_path, SOCKET_PATH, sizeof(local_addr.sun_path) - 1);
    if (bind(listen_sock_fd, (struct sockaddr *)&local_addr,
             sizeof(local_addr)) < 0) {
      perror("  ERROR: Failed to bind socket");
      close(listen_sock_fd);
      listen_sock_fd = -1;
      return 0;
    }
    if (listen(listen_sock_fd, 1) < 0) {
      perror("  ERROR: Failed to listen on socket");
      close(listen_sock_fd);
      listen_sock_fd = -1;
      return 0;
    }
    fcntl(listen_sock_fd, F_SETFL, O_NONBLOCK);
  }
  return 1;
}

void errorOnStart() {
  printf("[Process 5] CRITICAL: Failed to initialize one or more IPC "
         "mechanisms. Terminating.\n");
  fflush(stderr);
  cleanup_ipc();
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <log_filename>\n", argv[0]);
    return EXIT_FAILURE;
  }
  const char *log_filename = argv[1];

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigterm;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  log_file = fopen(log_filename, "a");
  if (!log_file) {
    perror("Process 5: Failed to open log file");
    return EXIT_FAILURE;
  }
  setvbuf(log_file, NULL, _IOLBF, BUFSIZ);

  if (!openP2File() || !openP3File() || !openP4File()) {
    errorOnStart();
    return EXIT_FAILURE;
  }

  // --- Event Loop using poll() ---
  struct pollfd fds[4]; // <<<--- Размерът 4 е все още ОК (fifo_p2, fifo_p3,
                        // listener, client)
  int nfds = 0;
  int client_fds_index = -1;

  memset(fds, 0, sizeof(fds));

  // <<<--- ПРОМЯНА: Инициализация на pollfd масива ---
  if (fifo_p2_fd != -1) {
    fds[nfds].fd = fifo_p2_fd;
    fds[nfds].events = POLLIN;
    nfds++;
  }
  if (mq != (mqd_t)-1) { // <<<--- НОВО: Добавяне на message queue
    fds[nfds].fd = mq;
    fds[nfds].events = POLLIN;
    nfds++;
  }
  // if (mq != (mqd_t)-1) { ... } // <<<--- ПРЕМАХНАТО
  if (listen_sock_fd != -1) {
    fds[nfds].fd = listen_sock_fd;
    fds[nfds].events = POLLIN;
    nfds++;
  }
  for (int i = nfds; i < 4; ++i) {
    fds[i].fd = -1;
  }

  while (!terminate_flag) {
    int current_nfds = 0;
    for (int i = 0; i < 4; ++i) {
      if (fds[i].fd != -1)
        current_nfds++;
    }
    if (current_nfds == 0 && !terminate_flag) {
      printf("[Process 5] No active IPC descriptors left. Exiting loop.\n");
      fflush(stdout);
      break;
    }

    int poll_ret = poll(fds, 4, 500);

    if (poll_ret < 0) {
      if (errno == EINTR)
        continue;
      perror("Process 5: poll() failed");
      break;
    }
    if (poll_ret == 0)
      continue;

    int current_checked_fds = 0;
    for (int i = 0; i < 4 && current_checked_fds < poll_ret; ++i) {

      if (fds[i].fd == -1 || fds[i].revents == 0)
        continue;
      current_checked_fds++;

      // Check Listening Socket
      if (fds[i].fd == listen_sock_fd && (fds[i].revents & POLLIN)) {
        // ... (кодът за accept остава същият) ...
        if (client_sock_fd == -1) {
          struct sockaddr_un remote_addr;
          socklen_t addr_len = sizeof(remote_addr);
          client_sock_fd = accept(listen_sock_fd,
                                  (struct sockaddr *)&remote_addr, &addr_len);
          if (client_sock_fd >= 0) {
            printf("[Process 5] Accepted connection from Process 4 (Socket FD: "
                   "%d)\n",
                   client_sock_fd);
            fflush(stdout);
            fcntl(client_sock_fd, F_SETFL, O_NONBLOCK);
            client_fds_index = -1; // Reset index
            for (int j = 0; j < 4; ++j) {
              if (fds[j].fd == -1) {
                fds[j].fd = client_sock_fd;
                fds[j].events = POLLIN | POLLHUP;
                client_fds_index = j;
                break;
              }
            }
            if (client_fds_index == -1) {
              close(client_sock_fd);
              client_sock_fd = -1;
            }
            // close(listen_sock_fd); fds[i].fd = -1; listen_sock_fd = -1;
          } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Process 5: Failed to accept connection");
          }
        } else {
          int temp_fd = accept(listen_sock_fd, NULL, NULL);
          if (temp_fd >= 0)
            close(temp_fd);
        } // Reject extra connections
      }

      // Check FIFO P2
      else if (fds[i].fd == fifo_p2_fd && (fds[i].revents & POLLIN)) {
        fifo_msg_int fifo_msg;
        ssize_t bytes_read = read(fifo_p2_fd, &fifo_msg, sizeof(fifo_msg));
        if (bytes_read == sizeof(fifo_msg)) {
          printf("[Process 5] Received from P2 (FIFO, PID %d): %d\n",
                 fifo_msg.source_pid, fifo_msg.value);
          fflush(stdout);
          if (log_file)
            fprintf(log_file, "[%ld] P2(%d): %d\n", time(NULL),
                    fifo_msg.source_pid, fifo_msg.value);
          fflush(log_file);
        } else if (bytes_read == 0) { // EOF
          printf("[Process 5] Process 2 (FIFO P2) closed connection.\n");
          fflush(stdout);
          close(fifo_p2_fd);
          fds[i].fd = -1;
          fifo_p2_fd = -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) { // Error
          perror("Process 5: Error reading from FIFO P2");
          close(fifo_p2_fd);
          fds[i].fd = -1;
          fifo_p2_fd = -1;
        }
      }

      // Check Message Queue <<<--- НОВА ЛОГИКА ---
      else if (fds[i].fd == mq && (fds[i].revents & POLLIN)) {
        mq_msg_float mq_msg;
        ssize_t bytes_read = mq_receive(mq, (char *)&mq_msg, sizeof(mq_msg),
                                        NULL); // No priority
        if (bytes_read == sizeof(mq_msg)) {
          printf("[Process 5] Received from P3 (Message Queue, PID %d): %f\n",
                 mq_msg.source_pid, mq_msg.value);
          fflush(stdout);
          if (log_file)
            fprintf(log_file, "[%ld] P3(%d): %f\n", (long)time(NULL),
                    mq_msg.source_pid, mq_msg.value);
          fflush(log_file);
        } else {
          perror("Process 5: Error receiving from message queue");
        }
      }

      // Check Message Queue <<<--- ПРЕМАХНАТО ---
      // else if (fds[i].fd == mq && ...) { ... }

      // Check Client Socket
      else if (fds[i].fd == client_sock_fd && client_sock_fd != -1) {
        if (fds[i].revents & (POLLHUP | POLLERR)) {
          printf("[Process 5] Error/Hangup on Process 4 socket connection (FD "
                 "%d).\n",
                 client_sock_fd);
          fflush(stdout);
          close(client_sock_fd);
          fds[i].fd = -1;
          client_sock_fd = -1;
          client_fds_index = -1;
        } else if (fds[i].revents & POLLIN) {
          sock_msg_string sock_msg;
          ssize_t bytes_received =
              recv(client_sock_fd, &sock_msg, sizeof(sock_msg), 0);
          if (bytes_received == sizeof(sock_msg)) {
            printf("[Process 5] Received from P4 (Socket, PID %d): \"%s\"\n",
                   sock_msg.source_pid, sock_msg.value);
            fflush(stdout);
          } else if (bytes_received == 0) {
            printf("[Process 5] Process 4 (Socket FD %d) closed connection.\n",
                   client_sock_fd);
            fflush(stdout);
            close(client_sock_fd);
            fds[i].fd = -1;
            client_sock_fd = -1;
            client_fds_index = -1;
          } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Process 5: recv failed");
            close(client_sock_fd);
            fds[i].fd = -1;
            client_sock_fd = -1;
            client_fds_index = -1;
          }
        }
      }
    }
  }

  printf("[Process 5] Exited event loop. Cleaning up...\n");
  fflush(stdout);
  cleanup_ipc();

  printf("[Process 5] Finished.\n");
  fflush(stdout);
  return EXIT_SUCCESS;
}
