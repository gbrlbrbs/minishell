#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <glob.h>
#include "parser.h"
#include "process.h"

#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITERS " \t\n\r\a"
#define PATH_BUFSIZE 1024
#define COMMAND_BUFSIZE 1024

#define COMMAND_EXTERNAL 0
#define COMMAND_EXIT 1
#define COMMAND_CD 2
#define COMMAND_EXPORT 3
#define COMMAND_UNSET 4

typedef struct shell_info {
    char pw_dir[PATH_BUFSIZE];
    char cur_user[TOKEN_BUFSIZE];
    char cur_dir[PATH_BUFSIZE];
} shell_info;

shell_info *init_shell();
void shell_print_welcome();
void shell_loop(shell_info *shell);
void update_cwd_info();
void print_prompt();
int get_command_type(char *command);
void launch_command(char *command);
void launch_builtin_command(int argc, char *argv[], int command_type, shell_info *shell);

#endif