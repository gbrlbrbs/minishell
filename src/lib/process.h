#include <stdlib.h>
#include <string.h>

typedef struct process {
    struct process* next;
    char** argv;
    pid_t pid;
    char completed;
    char stopped;
    int status;
} process;

void print_process();