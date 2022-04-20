#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void init_shell(pid_t* shell_pgid, struct termios shell_tmodes, int* shell_terminal, int* shell_is_interactive);

#endif