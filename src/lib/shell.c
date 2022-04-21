#include "shell.h"

void init_shell(pid_t* shell_pgid, struct termios* shell_tmodes, int* shell_terminal, int* shell_is_interactive) {
    *shell_terminal = STDIN_FILENO;
    *shell_is_interactive = isatty(*shell_terminal);
    if (*shell_is_interactive) {
        do {
            *shell_pgid = getpgrp();
            kill(- (*shell_pgid), SIGTTIN);
        } while (tcgetpgrp(*shell_terminal) != *shell_pgid);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        *shell_pgid = getpid();
        if (setpgid(*shell_pgid, *shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group.\n");
            exit(1);
        }

        tcsetpgrp(*shell_terminal, *shell_pgid);
        tcgetattr(*shell_terminal, shell_tmodes);
    }
}