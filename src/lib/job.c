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

