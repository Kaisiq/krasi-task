#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

// --- IPC Definitions ---
#define FIFO_PATH_P2 "./p5_fifo_p2"  // <<<--- ПРОМЯНА: Преименуван за яснота (Process 2 -> Process 5)
#define FIFO_PATH_P3 "./p5_fifo_p3"  // <<<--- НОВО: FIFO за Process 3 (Process 3 -> Process 5)
#define SOCKET_PATH "./p5_socket"    // Process 4 -> Process 5 (Unix Domain Socket)

// --- FIFO Structures ---
typedef struct {
    int source_pid;
    int value;
} fifo_msg_int; // За Процес 2

typedef struct {
    int source_pid;
    double value;
} fifo_msg_float; // <<<--- НОВО: За Процес 3

// --- Socket ---
#define SOCK_MSG_MAX_LEN 256
typedef struct {
    int source_pid;
    char value[SOCK_MSG_MAX_LEN];
} sock_msg_string;


// --- Message Queue (ПРЕМАХНАТО) ---
// #define MSGQ_NAME "/p5_mq"
// #define MSG_MAX_SIZE 1024
// #define MSGQ_PERMS 0666
// typedef struct {
//     long mtype;
//     int source_pid;
//     double value;
// } mq_msg_float;


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

#endif // COMMON_H
