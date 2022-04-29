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
    return shell;
}

void update_cwd_info(shell_info *shell) {
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

void shell_print_welcome() {
    printf("Minishell by gbrlbrbs.\n");
}

int launch_process_wrapper(process *proc, int infile, int outfile, shell_info *shell) {
    if (proc->command_type != COMMAND_EXTERNAL) {
        launch_builtin_command(proc->argc, proc->argv, proc->command_type, shell);
        return 0;
    }

    pid_t childpid;

    childpid = fork();
    if (childpid < 0) {
        perror("fork");
        return 1;
    } else if (childpid == 0) {
        launch_process(proc, infile, outfile);
        exit(EXIT_SUCCESS);
    } else {
        /* parent process */
        proc->pid = childpid;
        int status;
        do {
            waitpid(childpid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return status;
    }
}

int launch_parsed(parse_info *root, shell_info *shell) {
    parse_info *p;
    int status = 0, infile= 0, outfile, pipearr[2];
    for (p = root; p; p = p->next) {
        process *proc = parse_command_segment(p->segment);
        /* fork / launch process */
        if (p == root && proc->input_path) {
            infile = open(proc->input_path, O_RDONLY);
            if (infile < 0) {
                printf("minishell: no such file or directory: %s\n", proc->input_path);
                return -1;
            }
        }

        if (p->next) {
            if (pipe(pipearr) < 0) {
                perror("pipe");
                exit(1);
            }
            outfile = pipearr[1];
            launch_process_wrapper(proc, infile, outfile, shell);
            close(outfile);
            infile = pipearr[0];
        } else {
            outfile = 1;
            if (proc->output_path) {
                outfile = open(proc->output_path, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                if (outfile < 0) outfile = 1;
            }
            launch_process_wrapper(proc, infile, outfile, shell);
        }
    }
}

void shell_loop(shell_info *shell) {
    char *line;
    int status = 1;
    parse_info *parsed;
    while (true) {
        print_prompt(shell);
        line = readline();
        if (strlen(line) == 0) {
            continue;
        }
        parsed = parse_line(line);
        launch_parsed(parsed, shell);
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

void launch_builtin_command(int argc, char *argv[], int command_type, shell_info *shell) {
    switch (command_type) {
        case COMMAND_EXIT:
            shell_exit();
            break;
        case COMMAND_CD:
            shell_cd(argc, argv, shell);
            update_cwd_info(shell);
            break;
        case COMMAND_EXPORT:
            shell_export(argc, argv);
            break;
        case COMMAND_UNSET:
            shell_unset(argc, argv);
            break;
        default:
            break;
    }
}