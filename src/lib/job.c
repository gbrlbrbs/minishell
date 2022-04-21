#include "job.h"

/* const pointer for safety reasons */
job* find_job(pid_t pgid, job* const head) {
    job* j;
    for (j = head; j; j = j->next) {
        if (j->pgid == pgid) return j;
    }
    return NULL;
}

bool job_stopped(job* const j) {
    process* p;
    for (p = j->first_process; p; p = p->next) {
        if (!p->stopped && !p->completed) return false;
    }
    return true;
}

bool job_completed(job* const j) {
    process* p;
    for (p = j->first_process; p; p = p->next) {
        if (!p->completed) return false;
    }
    return true;
}

void launch_job(job* j, int foreground, int shell_is_interactive, int shell_terminal) {
    process* p;
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