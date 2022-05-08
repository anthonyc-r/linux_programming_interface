/**
* Toy scripting program to drive vi. Uses reentrant hash table gnu extension.
*  Usage: ./viscript script.vis
* Where script.vis is a file containing a list of the follow commands,
* separated by newlines.
* Max script length: 1000 lines, max line length: 100 characters. See defines.
* Commands:
*** CONTROL FLOW ***
* label <label_identifier>
* goto <label_identifier>
*** VARIABLES ***
* define <variable>
* set <variable> <value>
* decrement <variable>
* increment <variable>
*** OUTPUT ***
* open <filename>
* write <string>
* save
*/

#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <search.h>
#include <sys/ioctl.h>


#define MAX_SCRIPT 1000
#define MAX_LINE 100
#define MAX_VARS 100
#define MAX_LABS 100

enum command_type {
    COMMAND_TYPE_LABEL,
    COMMAND_TYPE_GOTO,
    COMMAND_TYPE_SET,
    COMMAND_TYPE_DECR,
    COMMAND_TYPE_INCR,
    COMMAND_TYPE_IF,
    COMMAND_TYPE_OPEN,
    COMMAND_TYPE_WRITE,
    COMMAND_TYPE_SAVE,
    COMMAND_TYPE_EOF
};

enum vim_exit {
    VIM_EXIT_SUCCESS = 1,
    VIM_EXIT_FAILURE
};

struct command {
    enum command_type type;
    char param[MAX_LINE];
};

static struct termios tios_restore;
static volatile sig_atomic_t vexit;
static struct hsearch_data varstab;
static struct hsearch_data labstab;

static void exit_err(char *reason) {
    perror(reason);
    exit(EXIT_FAILURE);
}

static void restore_tios() {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios_restore);
}

static void setraw() {
	struct termios raw;

	if (tcgetattr(STDOUT_FILENO, &tios_restore) == -1)
		exit_err("tcgetattr");
	if (atexit(restore_tios) != 0)
		exit_err("atexit");
	raw = tios_restore;
	raw.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
	raw.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR |
		INPCK | ISTRIP | IXON | PARMRK);
	raw.c_oflag &= ~OPOST;
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDOUT_FILENO, TCSADRAIN, &raw) == -1)
		exit_err("tcsetattr raw");
}

static int winchpt(int master) {
	struct winsize sz;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &sz) == -1)
		return -1;
	if (ioctl(master, TIOCSWINSZ, &sz) == -1)
		return -1;
}

static void handle_sig(int sig) {
    int wstat;

    if (wait(&wstat) == -1) {
        write(STDOUT_FILENO, "wstat\n", 6);
        vexit = VIM_EXIT_FAILURE;
    } else if (WEXITSTATUS(wstat) == EXIT_FAILURE) {
        write(STDOUT_FILENO, "exec vim\n", 9);
        vexit = VIM_EXIT_FAILURE;
    } else {
        vexit = VIM_EXIT_SUCCESS;
    }
}

static int parse_command_line(int fd, struct command *command) {
    char line[MAX_LINE];
    int i;
    ssize_t nread;

    for (i = 0; i < MAX_LINE - 1; i++) {
        nread = read(fd, line + i, 1);
        if (nread == -1) {
            return -1;
        } else if (nread == 0 && i == 0) {
            command->type = COMMAND_TYPE_EOF;
            command->param[0] = '\0';
            return 0;
        } else if (nread == 0) {
            line[i + 1] = '\0';
            break;
        } else if (line[i] == '\r') {
            exit_err("crlf not supported.");
        } else if (line[i] == '\n') {
            line[i] = '\0';
            break;
        }
    }
    if (strncmp("label ", line, 6) == 0) {
        command->type = COMMAND_TYPE_LABEL;
        strncpy(command->param, line + 6, MAX_LINE);
    } else if (strncmp("goto ", line, 5) == 0) {
        command->type = COMMAND_TYPE_GOTO;
        strncpy(command->param, line + 5, MAX_LINE);
    } else if (strncmp("set ", line, 4) == 0) {
        command->type = COMMAND_TYPE_SET;
        strncpy(command->param, line + 4, MAX_LINE);
    } else if (strncmp("decr ", line, 5) == 0) {
        command->type = COMMAND_TYPE_DECR;
        strncpy(command->param, line + 5, MAX_LINE);
    } else if (strncmp("incr ", line, 5) == 0) {
        command->type = COMMAND_TYPE_INCR;
        strncpy(command->param, line + 5, MAX_LINE);
    } else if (strncmp("open ", line, 5) == 0) {
        command->type = COMMAND_TYPE_OPEN;
        strncpy(command->param, line + 5, MAX_LINE);
    } else if (strncmp("write ", line, 6) == 0) {
        command->type = COMMAND_TYPE_WRITE;
        strncpy(command->param, line + 6, MAX_LINE);
    } else if (strncmp("save", line, 4) == 0) {
        command->type = COMMAND_TYPE_SAVE;
        command->param[0] = '\0';
    } else if (strncmp("if ", line, 3) == 0) {
        command->type = COMMAND_TYPE_IF;
        strncpy(command->param, line + 3, MAX_LINE);
    } else {
        errno = EINVAL;
        return -1;
    }
    if (command->param[MAX_LINE - 1] != '\0') {
        errno = E2BIG;
        return -1;
    }
    return 0;
}

static int parse_commands(int fd, struct command *out, int size) {
    int i;

    for (i = 0; i < size; i++) {
        if (parse_command_line(fd, out + i) == -1)
            return -1;
        if ((out + i)->type == COMMAND_TYPE_EOF)
            return i;
    }
    return i;
}

static int process_labels(struct command *script, int len) {
    int i;
    struct command *cmd;
    ENTRY ent, *eres;

    if (hcreate_r(MAX_LABS, &labstab) == 0)
        return -1;
    
    for (i = 0; i < len; i++) {
        cmd = script + i;
        if (cmd->type == COMMAND_TYPE_LABEL) {
            ent.key = cmd->param;
            ent.data = (void*)(long)i;
            if (hsearch_r(ent, ENTER, &eres, &labstab) == 0)
                return -1;
        }
    }
    return 0;
}

static int advance_script(struct command *script, int *loc, int len, int fd) {
    struct command *command = script + *loc;
    char buf[MAX_LINE], *cmpval, *lhs, *rhs, *key;
    int cmpres, ival;
    ENTRY ent, *eres;

    buf[0] = 27;
    write(fd, buf, 1);
    switch (command->type) {
    case COMMAND_TYPE_OPEN:
        strcpy(buf, ":o ");
        strncat(buf, command->param, MAX_LINE - (strlen(buf) + 1));
        strncat(buf, "\n", MAX_LINE - (strlen(buf) + 1));
        write(fd, buf, strlen(buf));
        break;
    case COMMAND_TYPE_SAVE:
        strcpy(buf, ":w\n");
        write(fd, buf, 3);
        break;
    case COMMAND_TYPE_WRITE:
        write(fd, "a", 1);
        write(fd, command->param, strlen(command->param));
        break;
    case COMMAND_TYPE_EOF:
        write(fd, ":q\n", 3);
        return 0;
    case COMMAND_TYPE_GOTO:
        ent.key = command->param;
        if (hsearch_r(ent, FIND, &eres, &labstab) == 0) {
            puts("label not found");
            return -1;
        }
        *loc = (long)(eres->data);
        break;
    case COMMAND_TYPE_SET:
        key = malloc(strlen(command->param));
        strncpy(key, command->param, MAX_LINE);
        ent.key = strtok(key, "=");
        ent.data = (void*)strtok(NULL, "=");
        if (hsearch_r(ent, ENTER, &eres, &varstab) == 0) {
            puts("add varstab");
            return -1;
        }
        break;
    case COMMAND_TYPE_IF:
        strncpy(buf, command->param, MAX_LINE);
        lhs = strtok(buf, "=><");
        rhs = strtok(NULL, "=><");
        ent.key = lhs;
        if (hsearch_r(ent, FIND, &eres, &varstab) == 0) {
            cmpval = lhs;
        } else {
            cmpval = (char*)eres->data;
        }
        cmpres = strcmp(rhs, cmpval);
        if (strchr(command->param, '=') && cmpres == 0) {
            *loc += 1;
        } else if (strchr(command->param, '<') && cmpres < 0)  {
            *loc += 1;
        } else if (strchr(command->param, '>') && cmpres > 0) {
            *loc += 1;
        }
        break;
    case COMMAND_TYPE_DECR:
        ent.key = command->param;
        if (hsearch_r(ent, FIND, &eres, &varstab) != 0) {
            ival = atoi((char*)(eres->data));
            ival--;
            snprintf(eres->data, MAX_LINE - (strlen(ent.key) + 1), "%d", ival);
        }
        break;
    case COMMAND_TYPE_INCR:
        ent.key = command->param;
        if (hsearch_r(ent, FIND, &eres, &varstab) != 0) {
            ival = atoi((char*)(eres->data));
            ival++;
            snprintf(eres->data, MAX_LINE - (strlen(ent.key) + 1), "%d", ival);
        }
        break;
    default:
        break;
    }
    *loc += 1;
    return 1;
}

static int spawn_vim(pid_t *child) {
    int master, slave;
    char *snam;

    if ((master = posix_openpt(O_RDWR)) == -1)
        return -1;
    if (grantpt(master) == -1)
        return -1;
    if (unlockpt(master) == -1)
        return -1;
    
    if (signal(SIGCHLD, handle_sig) == SIG_ERR)
        exit_err("sighandler");
    switch((*child = fork())) {
    case -1:
        return -1;
    case 0:
        if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
            exit(EXIT_FAILURE);
        if ((snam = ptsname(master)) == NULL)
            exit_err("ptsname");
        close(master);
        if (setsid() == (pid_t)-1)
		    exit_err("setsid");
        if ((slave = open(snam, O_RDWR)) == -1)
            exit_err("open slave");
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDERR_FILENO);
        execlp("vim", "vim", (char*)NULL);
        exit(EXIT_FAILURE);
    default:
        break;
    }
    return master;
}

static int pass_through(int in, int out) {
    int flags, nread;
    char buf[MAX_LINE];

    if ((flags = fcntl(in, F_GETFL)) == -1)
        exit_err("f_getfd");
    flags |= O_NONBLOCK;
    if (fcntl(in, F_SETFL, flags) == -1)
        exit_err("fnctl set flags");
    while ((nread = read(in, buf, MAX_LINE)) > 0)
        write(out, buf, nread);
    flags ^= O_NONBLOCK;
    fcntl(in, F_SETFL, flags);
}

static void drain(int fd) {
    char buf[MAX_LINE];
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1)
        exit_err("f_getfd");
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
        exit_err("fnctl set flags");
    while (read(fd, buf, MAX_LINE) > 0)
        ;
    flags ^= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

int main(int argc, char **argv) {
    struct command commands[MAX_SCRIPT];
    int fd, slen, sloc;
    pid_t child;

    if (argc < 2)
        exit_err("Usage: ./viscrpit <script_file>");
    if ((fd = open(argv[1], O_RDONLY)) == -1)
        exit_err("Open input");
    if ((slen = parse_commands(fd, commands, MAX_SCRIPT)) == -1)
        exit_err("parse commands");
    if  (process_labels(commands, slen) == -1)
        exit_err("process labels");
    if (hcreate_r(MAX_VARS, &varstab) == 0)
        exit_err("create vars table");
    close(fd);
    if ((fd = spawn_vim(&child)) == -1)
        exit_err("spawn vim");
    winchpt(fd);
    setraw();
    sloc = 0;
    while (advance_script(commands, &sloc, slen, fd) > 0) {
        pass_through(fd, STDOUT_FILENO);
        sleep(1);
        if (vexit != 0)
            break;
    }
    pass_through(fd, STDOUT_FILENO);
    drain(STDIN_FILENO);
    hdestroy_r(&varstab);
    hdestroy_r(&labstab);
    if (vexit == VIM_EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}