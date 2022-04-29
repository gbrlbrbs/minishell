#include "lib/shell.h"

int main (int argc, char* argv[]) {
    shell_info *shell = init_shell();
    shell_print_welcome(); 
    shell_loop(shell);
    return 0;
}