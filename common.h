#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>

// --- IPC Definitions ---
#define FIFO_PATH_P2 "./p5_fifo_p2"
#define MSGQ_NAME "/p5_mq_p3"
#define SOCKET_PATH "./p5_socket"

// --- FIFO Structures ---
typedef struct {
    int source_pid;
    int value;
} fifo_msg_int;


// --- Socket ---
#define SOCK_MSG_MAX_LEN 256
typedef struct {
    int source_pid;
    char value[SOCK_MSG_MAX_LEN];
} sock_msg_string;


#define MSG_MAX_SIZE 1024
#define MSGQ_PERMS 0666
typedef struct {
    long mtype;
    int source_pid;
    double value;
} mq_msg_float;


// --- Права за FIFO ---
#define FIFO_PERMS 0666 // <<<--- НОВО: Права за FIFO


// --- ANSI Color Codes ---
// Reset
#define COLOR_RESET   "\x1b[0m"
// Regular Text Colors
#define TXT_BLACK     "\x1b[0;30m"
#define TXT_RED       "\x1b[0;31m"
#define TXT_GREEN     "\x1b[0;32m"
#define TXT_YELLOW    "\x1b[0;33m"
#define TXT_BLUE      "\x1b[0;34m"
#define TXT_MAGENTA   "\x1b[0;35m"
#define TXT_CYAN      "\x1b[0;36m"
#define TXT_WHITE     "\x1b[0;37m"
// Background Colors
#define BG_BLACK      "\x1b[40m"
#define BG_RED        "\x1b[41m"
#define BG_GREEN      "\x1b[42m"
#define BG_YELLOW     "\x1b[43m"
#define BG_BLUE       "\x1b[44m"
#define BG_MAGENTA    "\x1b[45m"
#define BG_CYAN       "\x1b[46m"
#define BG_WHITE      "\x1b[47m"

// Helper to get color code string (simplified)
const char* get_color_code(int color_num, int is_bg) {
    // Simple mapping, can be expanded
    const char* txt_colors[] = {TXT_BLACK, TXT_RED, TXT_GREEN, TXT_YELLOW, TXT_BLUE, TXT_MAGENTA, TXT_CYAN, TXT_WHITE};
    const char* bg_colors[] = {BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE};
    int index = color_num % 8; // Map 0-7 to colors
    return is_bg ? bg_colors[index] : txt_colors[index];
}

int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}

int waitForKbhit(){
    for(int i=0; i<100; ++i){
        if(kbhit()){
            return 1;
        }
        usleep(100);
    }
    return 0;
}

#endif // COMMON_H
