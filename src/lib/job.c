#include "job.h"

/* const pointer for safety reasons */
job* find_job(pid_t pgid, job *const head) {
    job *j;
    for (j = head; j; j = j->next) {
        if (j->pgid == pgid) return j;
    }
    return NULL;
}

bool job_stopped(job *const j) {
    process* p;
    for (p = j->first_process; p; p = p->next) {
        if (!p->stopped && !p->completed) return false;
    }
    return true;
}

bool job_completed(job *const j) {
    process* p;
    for (p = j->first_process; p; p = p->next) {
        if (!p->completed) return false;
    }
    return true;
}

void launch_job(job *j, int foreground, int shell_is_interactive, int shell_terminal) {
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
        if (pid == 0) launch_process(p, j->pgid, infile, outfile, j->stderr, foreground, shell_is_interactive, shell_terminal);
        else if (pid < 0) {
            /* failed fork */
            perror("fork");
            exit(1);
        }
        else {
            /* parent process */
            p->pid = pid;
            if (shell_is_interactive) {
                if (!j->pgid) j->pgid = pid;
                setpgid(pid, j->pgid);
            }
        }

        if (infile != j->stdin) close(infile);

        if (outfile != j->stdout) close(outfile);

        infile = mypipe[0];
    }

    /* TODO: foreground/background and format info */
}

void put_job_in_foreground(job *j, int cont, int shell_terminal, pid_t shell_pgid, struct termios *shell_tmodes) {
    /* put job in foreground */
    tcsetpgrp(shell_terminal, j->pgid);

    if (cont) {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill(-(j->pgid), SIGCONT) < 0) perror("kill(SIGCONT)");
    }

    /* wait for job report */

    /* put shell back in foreground */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* restore shell terminal modes */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, shell_tmodes);
}

void put_job_in_background(job *j, int cont) {
    if (cont && kill(-(j->pgid), SIGCONT) < 0) perror("kill(SIGCONT)");
}

int mark_process_status(pid_t pid, int status, job *const head) {
    job *j;
    process *p;

    if (pid > 0) {
        for (j = head; j; j=j->next) {
            for (p = j->first_process; p; p=p->next) {
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED(status)) p->stopped = 1;
                    else {
                        p->completed = 1;
                        if (WIFSIGNALED(status)) 
                            fprintf(stderr,
                                "%d: Terminated by signal %d.\n",
                                pid, WTERMSIG(p->status)
                            );
                    }
                    return 0;
                } 
            }
        }
    }
}