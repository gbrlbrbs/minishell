#include "process.h"

void launch_process(
    process *p,
    pid_t pgid,
    int infile,
    int outfile,
    int errfile,
    int foreground,
    int shell_is_interactive,
    int shell_terminal
    ) {
    pid_t pid;
    if (shell_is_interactive) {
        pid = getpid();

        if (pgid == 0) pgid = pid;

        setpgid(pid, pgid);

        if (foreground) tcsetpgrp(shell_terminal, pgid);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
    }

    if (infile != STDIN_FILENO) {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO) {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if (errfile != STDERR_FILENO) {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }

    execvp(p->argv[0], p->argv);
    perror("execvp");
    exit(1);
}