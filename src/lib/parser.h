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

typedef struct parse_info {
    struct parse_info *next;
    char *segment;
} parse_info;

char *readline();
parse_info *parse_line(char *line);
char *strtrim(char *line);
process *parse_command_segment(char *segment);

#endif