#include "shell.h"

shell_info *init_shell() {
    pid_t pid = getpid();
    setpgid(pid, pid);
    tcsetpgrp(0, pid);

    shell_info *shell = (shell_info *) malloc(sizeof(shell_info));

    getlogin_r(shell->cur_user, sizeof(shell->cur_user));
    struct passwd *pw = getpwuid(getuid());
    strcpy(shell->pw_dir, pw->pw_dir);
    update_cwd_info(shell);
    shell->root_job = NULL;

    shell->shell_terminal = STDIN_FILENO;
    shell->is_interactive = isatty(shell->shell_terminal);
    if (shell->is_interactive) {
        while (tcgetpgrp (shell->shell_terminal) != (shell->shell_pgid = getpgrp ()))
            kill (- shell->shell_pgid, SIGTTIN);
        
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        shell->shell_pgid = getpid();
        if (setpgid (shell->shell_pgid, shell->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group.");
            exit(1);
        }

        /* grab control of the terminal */
        tcsetpgrp(shell->shell_terminal, shell->shell_pgid);

        /* save default terminal attributes for shell */
        tcgetattr(shell->shell_terminal, &(shell->shell_tmodes));

    }
    return shell;
}

int launch_process(process *p, int infile, int outfile, job* j, shell_info* shell) {
    if (p->command_type != COMMAND_EXTERNAL && launch_builtin_command(p, shell)) return 0;

    pid_t pid;

    if (shell->is_interactive) {
        /* 
        Put the process into the process group and give
        the process group the terminal if appropriate

        Has to be done both by the shell and in individual
        child process because of potential race conditions
        */

        pid = getpid();
        if (j->pgid == 0) j->pgid = pid;
        setpgid(pid, j->pgid);
        if (j->mode) tcsetpgrp(shell->shell_terminal, j->pgid);

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }
    
    if (infile != STDIN_FILENO) {
        dup2(infile, 0);
        close(infile);
    }

    if (outfile != STDOUT_FILENO) {
        dup2(outfile, 1);
        close(outfile);
    }

    if (execv(p->argv[0], p->argv) < 0) {
        printf("minishell: %s: command not found\n", p->argv[0]);
        exit(0);
    }

}

void format_job_info(job *j, const char *status) {
	fprintf(stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

void free_process(process *p) {
    int i;
    for (i = 0; p->argv[i]; ++i) {
        free(p->argv[i]);
    }
    free(p->argv);
    free(p);
}

void free_job(job *j) {
    process *next, *p;
    for (p = j->root_process; p; p = next) {
        next = p->next;
        free_process(p);
    }
    free(j->command);
    free(j);
}

job *find_job_by_pgid(pid_t pgid, shell_info *shell) {
    job *j;

    for (j = shell->root_job; j; j = j->next) {
        if (j->pgid == pgid) return j;
    }
    return NULL;
}

bool job_is_stopped(job *j) {
    process *p;

    for (p = j->root_process; p; p = p->next) {
        if (!p->completed && !p->stopped) return false;
    }
    return true;
}

bool job_is_completed(job *j) {
    process *p;

    for (p = j->root_process; p; p = p->next) {
        if (!p->completed) return false;
    }
    return true;
}

int mark_process_status(pid_t pid, int status, shell_info *shell) {
	job *j;
	process *p;

	if (pid > 0) {
		/* Update the record for the process.  */
		for (j = shell->root_job; j; j = j->next) {
			for (p = j->root_process; p; p = p->next) {
				if (p->pid == pid) {
					p->status = status;
					if (WIFSTOPPED(status)) {
						p->stopped = 1;
					} else {
						p->completed = 1;
						if (WIFSIGNALED(status)) {
							fprintf(stderr, "%d: Terminated by signal %d.\n",
									(int)pid, WTERMSIG(p->status));
						}
					}
					return 0;
				}
			}
		}
		fprintf(stderr, "No child process %d.\n", pid);
		return -1;
	} else if (pid == 0 || errno == ECHILD) {
		/* No processes ready to report.  */
		return -1;
	} else {
		/* Other weird errors.  */
		perror("waitpid");
		return -1;
	}
}

void print_job_info(job *j) {
    if (job_is_completed(j)) format_job_info(j, "completed");
    else if (job_is_stopped(j)) format_job_info(j, "stopped");
    else format_job_info(j, "running");
}

void wait_for_job(struct job *j, shell_info *shell) {
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WUNTRACED);
	} while (!mark_process_status(pid, status, shell) && !job_is_stopped(j) &&
			!job_is_completed(j));
}

void update_status(shell_info *shell) {
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
	} while (!mark_process_status(pid, status, shell));
}

void do_job_notification(shell_info *shell) {
	struct job *j, *jlast, *jnext;

	/* Update status information for child processes.  */
	update_status(shell);

	jlast = NULL;
	for (j = shell->root_job; j; j = jnext) {
		jnext = j->next;

		/* If all processes have completed, tell the user the job has
		   completed and delete it from the list of active jobs.  */
		if (job_is_completed(j)) {
			format_job_info(j, "completed");
			if (jlast) {
				jlast->next = jnext;
			} else {
				shell->root_job = jnext;
			}
			free_job(j);
		}
		/* Notify the user about stopped jobs,
		   marking them so that we won’t do this more than once.  */
		else if (job_is_stopped(j) && !j->notified) {
			format_job_info(j, "stopped");
			j->notified = 1;
			jlast = j;
		}
		/* Don’t say anything about jobs that are still running.  */
		else {
			jlast = j;
		}
	}
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */
void put_job_in_foreground(struct job *j, int cont, shell_info *shell) {
	/* Put the job into the foreground.  */
	tcsetpgrp(shell->shell_terminal, j->pgid);

	/* Send the job a continue signal, if necessary.  */
	if (cont) {
		tcsetattr(shell->shell_terminal, TCSADRAIN, &j->tmodes);
		if (kill(-j->pgid, SIGCONT) < 0) {
			perror("kill (SIGCONT)");
		}
	}

	/* Wait for it to report.  */
	wait_for_job(j, shell);

	/* Put the shell back in the foreground.  */
	tcsetpgrp(shell->shell_terminal, shell->shell_pgid);

	/* Restore the shell’s terminal modes.  */
	tcgetattr(shell->shell_terminal, &j->tmodes);
	tcsetattr(shell->shell_terminal, TCSADRAIN, &(shell->shell_tmodes));
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */
void put_job_in_background(struct job *j, int cont) {
	/* Send the job a continue signal, if necessary.  */
	if (cont) {
		if (kill(-j->pgid, SIGCONT) < 0) {
			perror("kill (SIGCONT)");
		}
	}
}

void update_cwd_info(shell_info *shell) {
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

void shell_print_welcome() {
    printf("Minishell by gbrlbrbs.\n");
}

int launch_job(job *j, shell_info *shell) {
    process *p;
    pid_t pid;
    int pipearr[2], infile, outfile;
    int status;

    infile = j->stdin;
    for (p = j->root_process; p; p = p->next) {
        if (p->next) {
            if (pipe(pipearr) < 0) {
                perror("pipe");
                exit(1);
            }
            outfile = pipearr[1];
        } else {
            outfile = j->stdout;
        }

        if (p->command_type != COMMAND_EXTERNAL) {
            status = launch_builtin_command(p, shell);
            p->completed = 1;
            return status;
        }

        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* child */
            launch_process(p, infile, outfile, j, shell);
        } else {
            p->pid = pid;
            if (shell->is_interactive) {
                if (!j->pgid) j->pgid = pid;
                setpgid(pid, j->pgid);
            }
        }

        if (infile != j->stdin) close(infile);
        if (outfile != j->stdout) close (outfile);

        infile = pipearr[0];
    }
    format_job_info(j, "launched");

    if (!shell->is_interactive) wait_for_job(j, shell);
    else if (j->mode == FOREGROUND_EXECUTION) put_job_in_foreground(j, 0, shell);
    else put_job_in_background(j, 0);
}

void shell_loop(shell_info *shell) {
    char *line;
    int status = 1;
    job *j;
    while (true) {
        do_job_notification(shell);
        print_prompt(shell);
        line = readline();
        if (strlen(line) == 0) {
            continue;
        }
        j = parse_line(line);
        job *next = shell->root_job;
        shell->root_job = j;
        j->next = next;
        status = launch_job(j, shell);
    }
}

void print_prompt(shell_info *shell) {
    printf("[%s %s] ", shell->cur_user, shell->cur_dir);
    printf("cmd> ");
}

int shell_exit() {
    printf("Exiting...\n");
    exit(EXIT_SUCCESS);
}

int shell_cd(int argc, char *argv[], shell_info *shell) {
    if (argc == 1) {
        chdir(shell->pw_dir);

        return 0;
    }

    if (chdir(argv[1]) == 0) {
        return 0;
    } else {
        printf("minishell: cd %s: No such file or directory\n", argv[1]);
        return 0;
    }
}

int shell_export(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: export KEY=VALUE\n");
        return -1;
    }

    return putenv(argv[1]);
}

int shell_unset(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: unset KEY\n");
        return -1;
    }

    return unsetenv(argv[1]);
}

int shell_jobs(shell_info *shell) {
    job *j;
    for (j = shell->root_job; j; j = j->next) {
        print_job_info(j);
    }
    return 0;
}

int shell_fg(int argc, char **argv, shell_info *shell) {
    if (argc < 2) {
        printf("usage: fg <pid>\n");
        return -1;
    }

    int status;
    pid_t pgid;
    pgid = atoi(argv[1]);

    if (pgid == 0) {
        printf("mysh: fg %s: no such job\n", argv[1]);
        return -1;
    }

    job *j = find_job_by_pgid(pgid, shell);

    if (kill(-pgid, SIGCONT) < 0) {
        printf("mysh: fg %d: job not found\n", pgid);
        return -1;
    }

    tcsetpgrp(0, pgid);

    wait_for_job(j, shell);

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(0, getpid());
    signal(SIGTTOU, SIG_DFL);

    return 0;
}

int shell_bg(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: bg <pid>\n");
        return -1;
    }

    pid_t pgid;
    pgid = atoi(argv[1]);

    if (pgid == 0) {
        printf("mysh: bg %s: no such job\n", argv[1]);
        return -1;
    }

    if (kill(-pgid, SIGCONT) < 0) {
        printf("mysh: bg %d: job not found\n", pgid);
        return -1;
    }

    return 0;
}

int shell_kill(int argc, char **argv, shell_info *shell) {
    if (argc < 2) {
        printf("usage: kill <pid>\n");
        return -1;
    }

    pid_t pgid;
    pgid = atoi(argv[1]);

    if (pgid == 0) {
        printf("mysh: kill %s: no such job\n", argv[1]);
        return -1;
    }

    job *j = find_job_by_pgid(pgid, shell);

    if (kill(pgid, SIGKILL) < 0) {
        printf("mysh: kill %d: job not found\n", pgid);
        return 0;
    }

    wait_for_job(j, shell);

    return 1;
}

int launch_builtin_command(process *p, shell_info *shell) {
    int status;
    switch (p->command_type) {
        case COMMAND_EXIT:
            status = shell_exit();
            break;
        case COMMAND_CD:
            status = shell_cd(p->argc, p->argv, shell);
            update_cwd_info(shell);
            break;
        case COMMAND_EXPORT:
            status = shell_export(p->argc, p->argv);
            break;
        case COMMAND_UNSET:
            status = shell_unset(p->argc, p->argv);
            break;
        case COMMAND_JOBS:
            status = shell_jobs(shell);
            break;
        case COMMAND_FG:
            status = shell_fg(p->argc, p->argv, shell);
            break;
        case COMMAND_BG:
            status = shell_bg(p->argc, p->argv);
            break;
        case COMMAND_KILL:
            status = shell_kill(p->argc, p->argv, shell);
            break;
        default:
            status = 0;
            break;
    }
    return status;
}