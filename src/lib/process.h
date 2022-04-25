#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

typedef struct process {
    struct process *next;
    char *command;
    int argc;
    char **argv;
    char *input_path;
    char *output_path;
    pid_t pid;
    int command_type;
    int completed;
    int stopped;
    int status;
} process;

void launch_process(process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground, int shell_is_interactive, int shell_terminal);