/**
 * Basic implementation of script(1).
 * Runs $SHELL under a pseudoterminal slave, the master then outputs
 * all input (where echoed) and output to the specified file.
 */

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>

#define BUFSZ 100

static struct termios tios_restore;

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void restore_tios() {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios_restore);
}

static void run_child(char *sname) {
	int slave;

	if (setsid() == (pid_t)-1)
		exit_err("setsid");
	if ((slave = open(sname, O_RDWR)) == -1)
		exit_err("open pt slave");
	if (ioctl(slave, TIOCSCTTY) == -1)
		exit_err("TIOCSTTY");
	if (dup2(slave, STDIN_FILENO) == -1)
		exit_err("dup2 STDIN");
	if (dup2(slave, STDOUT_FILENO) == -1)
		exit_err("dup2 STDOUT");
	if (dup2(slave, STDERR_FILENO) == -1) {
		puts("dup2 STDERR");
		exit_err("dup2 STDERR");
	}
	close(slave);
	execl("/bin/bash", "bash", (char*)NULL);
	exit_err("exec");
}

int main(int argc, char **argv) {
	int master, ofile;
	char *sname, buf[BUFSZ];
	struct pollfd fds[2];
	struct termios raw;
	ssize_t nread;

	if (argc < 2)
		exit_err("usage: ./script <output_file>");
	if ((ofile = open(argv[1], O_WRONLY | O_CREAT, S_IRWXU)) == -1)
		exit_err("failed to open output file");
	if ((master = posix_openpt(O_RDWR | O_NOCTTY)) < 0)
		exit_err("posix_openpt");
	if (grantpt(master) == -1)
		exit_err("grantpt");
	if ((sname = ptsname(master)) == NULL)
		exit_err("ptsname");
	if (unlockpt(master) == -1)
		exit_err("unlockpt");
	switch(fork()) {
		case -1:
			exit_err("fork");
		case 0:
			run_child(sname);
		default:
			break;
	}
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
#define FD_PT 0
#define FD_IO 1
	fds[FD_PT].fd = master;
	fds[FD_PT].events = POLLIN;
	fds[FD_IO].fd = STDIN_FILENO;
	fds[FD_IO].events = POLLIN;
	while (poll(fds, 2, -1) > 0) {
		if (fds[FD_PT].revents & POLLIN) {
			if ((nread = read(master, buf, BUFSZ)) > 0) {
				write(STDOUT_FILENO, buf, nread);
				write(ofile, buf, nread);
			}
		} else if (fds[FD_IO].revents & POLLIN) {
			if ((nread = read(STDIN_FILENO, buf, BUFSZ)) > 0)
				write(master, buf, nread);
		} else if (fds[FD_IO].revents & POLLHUP) {
			exit_err("STDIN HUP");
		} else if (fds[FD_PT].revents & POLLHUP) {
			exit_err("PT HUP");
		}
	}
	perror("poll");
#undef FD_PT
#undef FD_IO
	close(master);
	close(ofile);

	exit(EXIT_SUCCESS);
}
