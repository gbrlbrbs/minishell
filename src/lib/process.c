#include "process.h"

void launch_process(process *p, int infile, int outfile) {

    if (infile != 0) {
        dup2(infile, 0);
        close(infile);
    }

    if (outfile != 1) {
        dup2(outfile, 1);
        close(outfile);
    }

    if (execv(p->argv[0], p->argv) < 0) {
        printf("minishell: %s: command not found\n", p->argv[0]);
        exit(0);
    }

}
