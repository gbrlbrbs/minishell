#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <stdio.h>
#include <pwd.h>
#include <stdbool.h>
#include <glob.h>
#include "process.h"

#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITERS " \t\n\r\a"
#define COMMAND_BUFSIZE 1024

#define COMMAND_EXTERNAL 0
#define COMMAND_EXIT 1
#define COMMAND_CD 2
#define COMMAND_EXPORT 3
#define COMMAND_UNSET 4
#define COMMAND_JOBS 5
#define COMMAND_FG 6
#define COMMAND_BG 7
#define COMMAND_KILL 8

typedef struct job {
    char *command;
    int mode;
    process *root_process;
    pid_t pgid;
    struct termios tmodes;
    char notified;
    int stdin, stdout, stderr;
    struct job *next;
} job;

typedef struct parse_info {
    struct parse_info *next;
    char *segment;
} parse_info;

char *readline();
job *parse_line(char *line);
char *strtrim(char *line);
process *parse_command_segment(char *segment);

#endif