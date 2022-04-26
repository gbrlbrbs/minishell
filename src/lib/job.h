#ifndef JOB_H
#define JOB_H

#include "process.h"
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_CONTINUED 3
#define STATUS_TERMINATED 4

typedef struct job {
    struct job *next;
    char *command;
    process *first_process;
    pid_t pgid;
    char notified;
    struct termios tmodes;
    int foregound;
} job;

job *find_job(pid_t pgid, job *const head);
job *find_previous_job(job *j, job *const head);
bool job_stopped(job *const j);
bool job_completed(job *const j);
int mark_process_status(pid_t pid, int status, job *const head);
void wait_for_job(job *j, job *const head);
void format_job_info(job *const j, const char* status);
void do_job_notification(job *head);
void print_job_status(job *j);
process *find_process(pid_t pid, job *const head);
job* find_job_from_process_pid(pid_t pid, job *const head);
int set_process_shell_status(pid_t pid, int status, job *const head);
void update_status(job *const head);
void free_job(job *j);
void mark_job_as_running(job *j);

#endif