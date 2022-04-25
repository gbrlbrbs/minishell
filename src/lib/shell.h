#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdbool.h>
#include <glob.h>
#include "job.h"

#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITERS " \t\n\r\a"
#define PATH_BUFSIZE 1024
#define COMMAND_BUFSIZE 1024

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_CONTINUED 3
#define STATUS_TERMINATED 4

#define COMMAND_EXTERNAL 0
#define COMMAND_EXIT 1
#define COMMAND_CD 2
#define COMMAND_JOBS 3
#define COMMAND_FG 4
#define COMMAND_BG 5
#define COMMAND_KILL 6
#define COMMAND_EXPORT 7
#define COMMAND_UNSET 8

typedef struct shell_info {
    pid_t shell_pgid;
    struct termios shell_tmodes;
    int shell_terminal;
    int shell_is_interactive;
    char cur_user[TOKEN_BUFSIZE];
    char cur_dir[PATH_BUFSIZE];
    char pw_dir[PATH_BUFSIZE];
    job *jobs_head;
} shell_info;

void init_shell(shell_info *shell);
void shell_print_welcome();

#endif