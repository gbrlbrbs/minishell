#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct process {
    struct process* next;
    char** argv;
    pid_t pid;
    bool completed;
    bool stopped;
    int status;
} process;

void print_process();