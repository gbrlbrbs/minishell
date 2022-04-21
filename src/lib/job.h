#ifndef JOB_H
#define JOB_H
#include "process.h"
#include <termios.h>
#include <stdio.h>

typedef struct job {
    struct job* next;
    char* command;
    process* first_process;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int stdin, stdout, stderr;
} job;

job* find_job(pid_t pgid, job* const head);
/* stdbool imported in process.h */
bool job_stopped(job* const j);
bool job_completed(job* const j);

#endif