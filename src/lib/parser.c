#include "parser.h"

char *readline() {
    int bufsize = COMMAND_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "minishell: malloc error\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += COMMAND_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "minishell: malloc error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char *strtrim(char *line) {
    char *head = line;
    char *tail = line + strlen(line);

    while (*head == ' ') head++;

    while (*tail == ' ') tail++;

    *(tail + 1) = '\0';

    return head;
}

job* parse_line(char *line) {
    line = strtrim(line);

    char *command = strdup(line);

    process *root_proc = NULL, *proc = NULL;

    char *line_cursor = line;
    char *c = line;
    char *seg;

    int seg_len = 0, mode = FOREGROUND_EXECUTION;

    if (line[strlen(line) - 1] == '&') {
        mode = BACKGROUND_EXECUTION;
        line[strlen(line) -1] == '\0';
    }

    while (true) {
        if (*c == '|' || *c == '\0') {
            seg = (char *) malloc((seg_len + 1) * sizeof(char));
            strncpy(seg, line_cursor, seg_len);
            seg[seg_len] = '\0';

            process *new_proc = (process *) parse_command_segment(seg);
            if (!root_proc) {
                root_proc = new_proc;
                proc = root_proc;
            } else {
                proc->next = new_proc;
                proc = new_proc;
            }

            if (*c != '\0') {
                line_cursor = c;
                while (*(++line_cursor) == ' ');
                c = line_cursor;
                seg_len = 0;
                continue;
            } else {
                break;
            }
        } else {
            seg_len++;
            c++;
        }
    }

    job *new_job = (job *) malloc(sizeof(job));
    new_job->root_process = root_proc;
    new_job->command = command;
    new_job->mode = mode;
    new_job->pgid = -1;
    new_job->stdin = STDIN_FILENO;
    new_job->stdout = STDOUT_FILENO;
    new_job->stderr = STDERR_FILENO;
    return new_job;
}

int get_command_type(char *command) {
    if (strcmp(command, "exit") == 0) return COMMAND_EXIT;
    else if (strcmp(command, "cd") == 0) return COMMAND_CD;
    else if (strcmp(command, "export") == 0) return COMMAND_EXPORT;
    else if (strcmp(command, "unset") == 0) return COMMAND_UNSET;
    else if (strcmp(command, "jobs") == 0) return COMMAND_JOBS;
    else if (strcmp(command, "fg") == 0) return COMMAND_FG;
    else if (strcmp(command, "bg") == 0) return COMMAND_BG;
    else if (strcmp(command, "kill") == 0) return COMMAND_KILL;
    else return COMMAND_EXTERNAL;
}

process *parse_command_segment(char *segment) {
    int bufsize = TOKEN_BUFSIZE;
    int position = 0;
    char *command = strdup(segment);
    char *token;
    char **tokens = (char**) malloc(bufsize * sizeof(char*));

    if (!tokens) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(segment, TOKEN_DELIMITERS);
    while (token != NULL) {
        glob_t glob_buffer;
        int glob_count = 0;
        if (strchr(token, '*') != NULL || strchr(token, '?') != NULL) {
            glob(token, 0, NULL, &glob_buffer);
            glob_count = glob_buffer.gl_pathc;
        }

        if (position + glob_count >= bufsize) {
            bufsize += TOKEN_BUFSIZE;
            bufsize += glob_count;
            tokens = (char**) realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "minishell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        if (glob_count > 0) {
            int i;
            for (i = 0; i < glob_count; i++) {
                tokens[position++] = strdup(glob_buffer.gl_pathv[i]);
            }
            globfree(&glob_buffer);
        } else {
            tokens[position] = token;
            position++;
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }


    int i = 0, argc = 0;
    char *input_path = NULL, *output_path = NULL;
    while (i < position) {
        if (tokens[i][0] == '<' || tokens[i][0] == '>') {
            break;
        }
        i++;
    }
    argc = i;

    for (; i < position; i++) {
        if (tokens[i][0] == '<') {
            if (strlen(tokens[i]) == 1) {
                input_path = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(input_path, tokens[i + 1]);
                i++;
            } else {
                input_path = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(input_path, tokens[i] + 1);
            }
        } else if (tokens[i][0] == '>') {
            if (strlen(tokens[i]) == 1) {
                output_path = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(output_path, tokens[i + 1]);
                i++;
            } else {
                output_path = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(output_path, tokens[i] + 1);
            }
        } else {
            break;
        }
    }

    for (i = argc; i <= position; i++) {
        tokens[i] = NULL;
    }

    process *new_process = (process *) malloc(sizeof(process));
    new_process->argc = argc;
    new_process->argv = tokens;
    new_process->command = command;
    new_process->input_path = input_path;
    new_process->output_path = output_path;
    new_process->pid = -1;
    new_process->command_type = get_command_type(tokens[0]);
    return new_process;
}
