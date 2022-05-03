/**
* Unbuffer the target program. Test with longrunner.c
* ./unbuffer longrunner | grep jdofisskd
*/

#define _XOPEN_SOURCE 600

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#define BUFSZ 100

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void xfer(int in, int out) {
    char buf[BUFSZ];
    ssize_t nread;

    if ((nread = read(in, buf, BUFSZ)) < 1)
        exit_err("read fd");
    if (write(out, buf, nread) != nread)
        exit_err("write master");
}

static void pass_through(int master) {
    ssize_t nread;
	struct pollfd polls[3];

#define FD_IN 0
#define FD_OUT 1
#define FD_PT 2
	polls[FD_IN].fd = STDIN_FILENO;
	polls[FD_IN].events = POLLIN;
	polls[FD_OUT].fd = STDOUT_FILENO;
	polls[FD_OUT].events = POLLIN;
	polls[FD_PT].fd = master;
	polls[FD_PT].events = POLLIN | POLLOUT;
	while ((nread = poll(polls, 3, -1)) > 0) {
		if (polls[FD_PT].revents & POLLOUT == POLLOUT)
            xfer(master, STDOUT_FILENO);
		else if (polls[FD_PT].revents & POLLIN == POLLIN)
			xfer(STDIN_FILENO, master);
        else if (polls[FD_IN].revents & POLLOUT == POLLOUT)
            xfer(STDIN_FILENO, master);
        else if (polls[FD_OUT].revents & POLLIN == POLLIN)
            xfer(master, STDOUT_FILENO);
	}
#undef FDA
#undef FDB
}

static void handler(int sig) {
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    int master, slave;
    char *sname;
    
    if (argc < 2)
        exit_err("./unbuffer prog");

    if ((master = posix_openpt(O_RDWR)) == -1)
        exit_err("posix_openpt");
    if (unlockpt(master) == -1)
        exit_err("unlock_pt");
    if (grantpt(master) == -1)
        exit_err("grantpt");
    switch (fork()) {
    case -1:
        exit_err("fork");
    case 0:
        if ((sname = ptsname(master)) == NULL)
            exit_err("ptsname");
        close(master);
        if ((slave = open(sname, O_RDWR)) == -1)
            exit_err("open slave");
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        execl(argv[1], argv[1], (char*)NULL);
        exit_err("exec");
    default:
        break;    
    }
    signal(SIGCHLD, handler);
    pass_through(master);
    exit(EXIT_SUCCESS);
}