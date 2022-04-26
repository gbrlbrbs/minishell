#include "lib/shell.h"

int main (int argc, char* argv[]) {
    shell_info *shell = (shell_info *) malloc(sizeof(shell_info));
    init_shell(shell);
    shell_print_welcome(); 
    shell_loop(shell);
    return 0;
}