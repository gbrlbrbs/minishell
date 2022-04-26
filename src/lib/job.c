#include "job.h"

const char *STATUS_STRING[] = {
    "running",
    "done",
    "suspended",
    "continued",
    "terminated"
};

/* const pointer for safety reasons */
job* find_job(pid_t pgid, job *const head) {
    job *j;
    for (j = head; j; j = j->next) {
        if (j->pgid == pgid) return j;
    }
    return NULL;
}

job *find_previous_job(job *j, job *const head) {
    job *lastj;
    if (j == head) return j;
    for (lastj = head; lastj; lastj = lastj->next) {
        if (lastj->next == j) return lastj;
    }
    return NULL;
}

process *find_process(pid_t pid, job *const head) {
    process *p;
    job *j;
    for (j = head; j; j = j->next) {
        for (p = j->first_process; p; p = p->next) {
            if (p->pid == pid) return p;
        }
    }
    return NULL;
}

job* find_job_from_process_pid(pid_t pid, job *const head) {
    process *p;
    job *j;
    for (j = head; j; j = j->next) {
        for (p = j->first_process; p; p = p->next) {
            if (p->pid == pid) return j;
        }
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

int mark_process_status(pid_t pid, int status, job *const head) {
    job *j;
    process *p;

    if (pid > 0) {
        for (j = head; j; j = j->next) {
            for (p = j->first_process; p; p = p->next) {
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED(status)) {
                        p->stopped = 1;
                        p->shell_status = STATUS_SUSPENDED;
                    } else {
                        p->completed = 1;
                        p->shell_status = STATUS_DONE;
                        if (WIFSIGNALED(status)) { 
                            fprintf(stderr,
                                "%d: Terminated by signal %d.\n",
                                pid, WTERMSIG(p->status)
                            );
                            p->shell_status = STATUS_TERMINATED;
                        }
                    }
                    return 0;
                } 
            }
        }
        /* it's here if it hasn't found any process */
        fprintf(stderr, "No child process with PID = %d", pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD) {
        return -1;
    } else {
        perror("waitpid");
        return -1;
    }
}

void update_status(job *const head) {
    int status;
    pid_t pid;
    do {
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
    } while (!mark_process_status(pid, status, head));
}

void wait_for_job(job *j, job *const head) {
    int status;
    pid_t pid;

    do {
        pid = waitpid(WAIT_ANY, &status, WUNTRACED);
    } while (!mark_process_status(pid, status, head) && !job_stopped(j) && !job_completed(j));
    
}

void format_job_info(job *const j, const char* status) {
    fprintf(stderr, "%ld (%s): %s\n", (long) j->pgid, status, j->command);
}

void do_job_notification(job *head) {
    job *j, *nextj, *lastj;

    update_status(head);

    lastj = NULL;
    for (j = head; j; j = nextj) {
        nextj = j->next;

        if (job_completed(j)) {
            format_job_info(j, "completed");
            if (lastj) lastj->next = nextj;
            else head = nextj;
            free_job(j);
        } else if (job_stopped(j) && !j->notified) {
            format_job_info(j, "stopped");
            j->notified = 1;
            lastj = j;
        } else {
            lastj = j;
        }
    }
}

void free_job(job *j) {
    process *p = j->first_process;
    process *lastp;

    while (p != NULL) {
        lastp = p;
        p = p->next;
        if (lastp->argv) free(lastp->argv);
        if (lastp->command) free(lastp->command);
        if (lastp->input_path) free(lastp->input_path);
        if (lastp->output_path) free(lastp->output_path);
        free(lastp);
    }
    free(j->command);
    free(j);
}

void mark_job_as_running(job *j) {
    process *p;

    for (p = j->first_process; p; p=p->next) {
        p->stopped = 0;
        p->shell_status = STATUS_RUNNING;
    }
    j->notified = 0;
}

void print_job_status(job *j) {
    process *p;
    for (p = j->first_process; p; p = p->next) {
        printf("[%d] %s %s", p->pid, STATUS_STRING[p->shell_status], p->command);
        if (p->next != NULL) printf("|\n");
        else printf("\n");
    }
}

int set_process_shell_status(pid_t pid, int status, job *const head) {
    job *j;
    process *p;

    if (pid > 0) {
        for (j = head; j; j = j->next) {
            for (p = j->first_process; p; p = p->next) {
                if (p->pid == pid) {
                    p->shell_status = status;
                    return 0;
                } 
            }
        }
        /* it's here if it hasn't found any process */
        fprintf(stderr, "No child process with PID = %d", pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD) {
        return -1;
    } else {
        perror("waitpid");
        return -1;
    }
}