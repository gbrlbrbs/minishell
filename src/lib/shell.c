#include "shell.h"

void init_shell(shell_info *shell) {
    shell->shell_terminal = STDIN_FILENO;
    shell->shell_is_interactive = isatty(shell->shell_terminal);
    if (shell->shell_is_interactive) {
        do {
            shell->shell_pgid = getpgrp();
            kill(- (shell->shell_pgid), SIGTTIN);
        } while (tcgetpgrp(shell->shell_terminal) != shell->shell_pgid);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        shell->shell_pgid = getpid();
        if (setpgid(shell->shell_pgid, shell->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group.\n");
            exit(EXIT_FAILURE);
        }

        tcsetpgrp(shell->shell_terminal, shell->shell_pgid);
        tcgetattr(shell->shell_terminal, &(shell->shell_tmodes));
    }
    getlogin_r(shell->cur_user, sizeof(shell->cur_user));
    struct passwd *pw = getpwuid(getuid());
    strcpy(shell->pw_dir, pw->pw_dir);
    shell->jobs_head = NULL;
    update_cwd_info(shell);
}

void update_cwd_info(shell_info *shell) {
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

void shell_print_welcome() {
    printf("Minishell by gbrlbrbs.\n");
}

void shell_loop(shell_info *shell) {
    char *line;
    job *j;
    int status = 1;
    while (true) {
        print_prompt(shell);
        line = readline();
        if (strlen(line) == 0) {
            check_zombie(shell);
            continue;
        }
        j = parse_command(line);
        launch_job(j, j->foregound, shell);
    }
}

job *parse_command(char *line) {
    line = strtrim(line);

    char *command = strdup(line);

    process *root_proc = NULL;
    process *p = NULL;

    char *line_cursor = line;
    char *c = line;
    char *seg;

    int seg_len = 0;
    int foreground = 1;

    if (line[strlen(line) - 1] == '&') {
        foreground = 0;
        line[strlen(line) - 1] = '\0';
    }

    while (true) {
        if (*c == '\0' || *c == '|') {
            seg = (char *) malloc((seg_len + 1) * sizeof(char));
            strncpy(seg, line_cursor, seg_len);
            seg[seg_len] = '\0';

            process *new_p = parse_command_segment(seg);

            if (!root_proc) {
                root_proc = new_p;
                p = root_proc;
            } else {
                p->next = new_p;
                p = new_p;
            }

            if (*c != '\0') {
                line_cursor = c;
                while (*(++line_cursor) == ' ');
                c = line_cursor;
                seg_len = 0;
                continue;
            } else break;
        } else {
            seg_len++;
            c++;
        }
    }

    job *new_job = (job *) malloc(sizeof(job));
    new_job->first_process = root_proc;
    new_job->command = command;
    new_job->pgid = -1;
    new_job->foregound = foreground;
    return new_job;
}

process *parse_command_segment(char *seg) {
    int bufsize = TOKEN_BUFSIZE;
    int position = 0;
    char *command = strdup(seg);
    char *token;
    char **tokens = (char **) malloc(bufsize * sizeof(char *));

    if (!tokens) {
        fprintf(stderr, "minishell: malloc error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(seg, TOKEN_DELIMITERS);
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
            tokens = (char **) realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "minishell: malloc error\n");
                exit(EXIT_FAILURE);
            }
        }

        if (glob_count > 0) {
            int i;
            for (i = 0; i < glob_count; i++) tokens[position++] = strdup(glob_buffer.gl_pathv[i]);
            globfree(&glob_buffer);
        } else {
            tokens[position] = token;
            position++;
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    int i = 0;
    int argc = 0;
    char *input_path = NULL;
    char *output_path = NULL;

    while (i < position) {
        if (tokens[i][0] == '>' || tokens[i][0] == '<') break;
        i++;
    }
    argc = i;

    for (; i < position; i++) {
        if (tokens[i][0] == '<') {
            if (strlen(tokens[i]) == 1) {
                input_path = (char *) malloc((strlen(tokens[i+1]) + 1) * sizeof(char));
                strcpy(input_path, tokens[i+1]);
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

    process *new_proc = (process *) malloc(sizeof(process));
    new_proc->command = command;
    new_proc->argc = argc;
    new_proc->argv = tokens;
    new_proc->input_path = input_path;
    new_proc->output_path = output_path;
    new_proc->pid = -1;
    new_proc->next = NULL;
    new_proc->command_type = get_command_type(command);
    return new_proc;
}

char *strtrim(char *line) {
    char *head = line;
    char *tail = line + strlen(line);

    while (*head == ' ') head++;

    while (*tail == ' ') tail++;

    *(tail + 1) = '\0';

    return head;
}

int get_command_type(char *command) {
    if (strcmp(command, "exit") == 0) return COMMAND_EXIT;
    else if (strcmp(command, "cd") == 0) return COMMAND_CD;
    else if (strcmp(command, "jobs") == 0) return COMMAND_JOBS;
    else if (strcmp(command, "fg") == 0) return COMMAND_FG;
    else if (strcmp(command, "bg") == 0) return COMMAND_BG;
    else if (strcmp(command, "kill") == 0) return COMMAND_KILL;
    else if (strcmp(command, "export") == 0) return COMMAND_EXPORT;
    else if (strcmp(command, "unset") == 0) return COMMAND_UNSET;
    else return COMMAND_EXTERNAL;
}

void print_prompt(shell_info *const shell) {
    printf("[%s %s] ", shell->cur_user, shell->cur_dir);
    printf("cmd> ");
}

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

void check_zombie(shell_info *const shell) {
    int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            mark_process_status(pid, STATUS_DONE, shell->jobs_head);
        } else if (WIFSTOPPED(status)) {
            mark_process_status(pid, STATUS_SUSPENDED, shell->jobs_head);
        } else if (WIFCONTINUED(status)) {
            mark_process_status(pid, STATUS_CONTINUED, shell->jobs_head);
        }
        
        if (pid > 0) {
            do_job_notification(shell->jobs_head);
        }
    }
}

void launch_job(job *j, int foreground, shell_info *shell) {
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;

    infile = j->stdin;
    for (p = j->first_process; p; p = p->next) {
        if (p->next) {
            if (pipe(mypipe) < 0) {
                perror("pipe");
                exit(1);
            }
            outfile = mypipe[1];
        } else outfile = j->stdout;

        /* create fork to launch the process */
        pid = fork();
        if (pid == 0) {
            if (p->command_type != COMMAND_EXTERNAL) launch_builtin_command(p);
            else launch_process(p, j->pgid, infile, outfile, j->stderr, foreground, shell->shell_is_interactive, shell->shell_terminal);
        } else if (pid < 0) {
            /* failed fork */
            perror("fork");
            exit(1);
        }
        else {
            /* parent process */
            p->pid = pid;
            if (shell->shell_is_interactive) {
                if (!j->pgid) j->pgid = pid;
                setpgid(pid, j->pgid);
            }
        }

        if (infile != j->stdin) close(infile);

        if (outfile != j->stdout) close(outfile);

        infile = mypipe[0];
    }

    format_job_info(j, "launched");
    if (!shell->shell_is_interactive) wait_for_job(j, shell->jobs_head);
    else if (foreground) put_job_in_foreground(j, 0, shell);
    else put_job_in_background(j, 0);
}

void put_job_in_foreground(job *j, int cont, shell_info *shell) {
    /* put job in foreground */
    tcsetpgrp(shell->shell_terminal, j->pgid);

    if (cont) {
        tcsetattr(shell->shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill(-(j->pgid), SIGCONT) < 0) perror("kill(SIGCONT)");
    }

    /* wait for job report */
    wait_for_job(j, shell->jobs_head);

    /* put shell back in foreground */
    tcsetpgrp(shell->shell_terminal, shell->shell_pgid);

    /* restore shell terminal modes */
    tcgetattr(shell->shell_terminal, &j->tmodes);
    tcsetattr(shell->shell_terminal, TCSADRAIN, &shell->shell_tmodes);
}

void put_job_in_background(job *j, int cont) {
    if (cont && kill(-(j->pgid), SIGCONT) < 0) perror("kill(SIGCONT)");
}

void continue_job(job *j, int foreground, shell_info *shell) {
    mark_job_as_running(j);
    if (foreground) put_job_in_foreground(j, 1, shell);
    else put_job_in_background(j, 1);
}