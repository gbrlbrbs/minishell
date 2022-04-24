#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/process.h"
#include "lib/shell.h"

int main (int argc, char* argv[]) {
    shell_info *shell = (shell_info *) malloc(sizeof(shell_info));
    init_shell(shell);
    /* shell_print_welcome(); */
    /* shell_loop(); */
    return 0;
}