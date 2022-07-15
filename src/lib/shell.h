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

#define PATH_BUFSIZE 1024

#define COMMAND_EXTERNAL 0
#define COMMAND_EXIT 1
#define COMMAND_CD 2
#define COMMAND_EXPORT 3
#define COMMAND_UNSET 4

#define PROC_FILTER_ALL 0
#define PROC_FILTER_DONE 1
#define PROC_FILTER_REMAINING 2

typedef struct shell_info {
    char pw_dir[PATH_BUFSIZE];
    char cur_user[TOKEN_BUFSIZE];
    char cur_dir[PATH_BUFSIZE];
    int is_interactive;
    int shell_terminal;
    struct termios shell_tmodes;
    pid_t shell_pgid;
    job *root_job;
} shell_info;

shell_info *init_shell();

int launch_process(process *p, int infile, int outfile, job *j, shell_info* shell);
void shell_print_welcome();
void shell_loop(shell_info *shell);
void update_cwd_info();
void print_prompt();
int get_command_type(char *command);
void launch_command(char *command);
int launch_builtin_command(process *p, shell_info *shell);

#endif