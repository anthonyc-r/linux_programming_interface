/**
* Simple scripting tool to drive vi. Uses reentrant hash table gnu extension.
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define _GNU_SOURCE
#include <search.h>


#define MAX_SCRIPT 1000
#define MAX_LINE 100

enum command_type {
    COMMAND_TYPE_LABEL,
    COMMAND_TYPE_GOTO,
    COMMAND_TYPE_DEF,
    COMMAND_TYPE_SET,
    COMMAND_TYPE_DECR,
    COMMAND_TYPE_INCR,
    COMMAND_TYPE_IF,
    COMMAND_TYPE_OPEN,
    COMMAND_TYPE_WRITE,
    COMMAND_TYPE_SAVE,
    COMMAND_TYPE_EOF
};

struct command {
    enum command_type type;
    char param[MAX_LINE];
};

struct map {
    pid_t pid;
    int mq;
};

static void exit_err(char *reason) {
    perror(reason);
    exit(EXIT_FAILURE);
}

static int parse_command_line(int fd, struct command *command) {
    char line[MAX_LINE];
    int i;
    ssize_t nread;

    for (i = 0; i < MAX_LINE; i++) {
        nread = read(fd, line + i, 1);
        if (nread == 0) {
            command->type = COMMAND_TYPE_EOF;
            command->param[0] = '\0';
            return 0;
        } else if (nread == -1)
            return -1;
        else if (line[i] == '\n') {
            line[i] = '\0';
            break;
        }
    }
    if (strncmp("label ", line, 7) == 0) {
        command->type = COMMAND_TYPE_LABEL;
        strncpy(command->param, line + 7, MAX_LINE);
    } else if (strncmp("goto ", line, 5) == 0) {
        command->type = COMMAND_TYPE_GOTO;
        strncpy(command->param, line + 5, MAX_LINE);
    } else if (strncmp("def ", line, 4) == 0) {
        command->type = COMMAND_TYPE_DEF;
        strncpy(command->param, line + 4, MAX_LINE);
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
    } else if (strncmp("save ", line, 5) == 0) {
        command->type = COMMAND_TYPE_SAVE;
        command->param[0] = '\0';
    } else {
        errno = EINVAL;
        return -1;
    }
    if (command->param[MAX_LINE - 1] != '\0') {
        errno = ERR2BIG;
        return -1;
    }
    return 0;
}

static int parse_commands(int fd, struct command *out, int size) {
    int i;

    for (i = 0; i < size; i++) {
        if (parse_command_line(fd, out + i) == -1)
            return -1;
    }
    return i;
}

static int advance_script(struct command *script, int *loc, int len, int fd) {
    struct command *command = script + loc;
    char buf[MAX_LINE];

    buf[0] = 27;
    write(fd, buf, 1);
    switch (command->type) {
    case COMMAND_TYPE_OPEN:
        strcpy(buf, ":o ");
        strncat(buf, command->param, MAX_LINE);
        strncat(buf, "\n", MAX_LINE);
    case COMMAND_TYPE_SAVE:
        strcpy(buf, ":w\n");
        write(fd, buf, 3);
    case COMMAND_TYPE_EOF:
        return 0;
    }
    return 1;
}


int main(int argc, char **argv) {
    struct command commands[MAX_SCRIPT];
    int fd, slen, sloc;

    if (argc < 2)
        exit_err("Usage: ./viscrpit <script_file>");
    if ((fd = open(argv[1], O_RDONLY)) == -1)
        exit_err("Open input");
    if ((slen = parse_commands(fd, commands, MAX_SCRIPT)) == -1)
        exit_err("parse commands");
    
    sloc = 0;
    while (advance_script(commands, &sloc, slen, fd) > 0)
        ;
    exit(EXIT_SUCCESS);
}