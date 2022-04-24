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
    }
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
            mark_process_status(pid, status, shell->jobs_head);
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
        if (pid == 0) launch_process(p, j->pgid, infile, outfile, j->stderr, foreground, shell->shell_is_interactive, shell->shell_terminal);
        else if (pid < 0) {
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