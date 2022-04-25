#ifndef JOB_H
#define JOB_H
#include "process.h"
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct job {
    struct job *next;
    char *command;
    process *first_process;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int stdin, stdout, stderr;
    int foregound;
} job;

job * find_job(pid_t pgid, job *const head);
/* stdbool imported in process.h */
bool job_stopped(job *const j);
bool job_completed(job *const j);
int mark_process_status(pid_t pid, int status, job *const head);
void wait_for_job(job *j, job *const head);
void format_job_info(job *const j, const char* status);
void do_job_notification(job *head);

#endif