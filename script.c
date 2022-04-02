/**
 * Replayable implementation of script(1).
 * Runs $SHELL under a pseudoterminal slave, the master then outputs
 * all input (where echoed) and output to the specified file. Each output is timestamped
 * so that it can be replayed.
 */

#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define BUFSZ 100

static struct termios tios_restore;
static volatile sig_atomic_t winch;
static unsigned long long smillis;

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

static int log_time(int ofile, int finished) {
	time_t t;
	char *str, *intro;
	size_t len;

	intro = finished ? "Script finished on: " : "Script started on: ";

	if (time(&t) == (time_t)-1)
		return -1;
	if ((str = ctime(&t)) == NULL)
		return -1;
	len = strlen(intro);
	if (write(ofile, intro, len) != (ssize_t)len)
		return -1;
	len = strlen(str);
	if (write(ofile, str, len) != (ssize_t)len)
		return -1;
}

static void handle(int sig) {
	winch = 1;
}

static int setup_winch() {
	struct sigaction sigact;

	memset(&sigact, 0, sizeof (sigact));
	sigact.sa_handler = handle;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(SIGWINCH, &sigact, NULL) == -1)
		return -1;
}

static int winchpt(int master) {
	struct winsize sz;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &sz) == -1)
		return -1;
	if (ioctl(master, TIOCSWINSZ, &sz) == -1)
		return -1;
}

static int get_millis(unsigned long long *out) {
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
		return -1;
	*out = (unsigned long long)ts.tv_sec * 1000L;
	*out += (unsigned long long)(ts.tv_nsec / 1000000L);
}
static int timestamp(int fd) {
	unsigned long long millis, diff;
	int nprint;

	if (get_millis(&millis) == -1)
		return -1;
	if (millis < smillis)
		return -1;
	diff = smillis - millis;
	write(fd, "\n", 1);
	write(fd, &diff, sizeof (diff));
	return 0;
}

static int escape(char *in, int ilen, char *out, int olen) {
	int i, j;
	char c;

	for (i = 0, j = 0; i < ilen && j < olen; j++, i++) {
		if (in[i] == '\n') {
			out[j++] = '\\';
			out[j] = 'n';
		} else if (i < (ilen - 1) && in[i + 1] == 'n' && in[i] == '\\') {
			out[j++] = '\\';
			out[j++] = '\\';
			out[j] = 'n';
		} else {
			out[j] = in[i];
		}
	}
	return j;
}

int main(int argc, char **argv) {
	int master, ofile, result;
	char *sname, buf[BUFSZ], escbuf[2 * BUFSZ];
	struct pollfd fds[2];
	struct termios raw;
	ssize_t nread;

	if (argc < 2)
		exit_err("usage: ./script <output_file>");
	if (get_millis(&smillis) == -1)
		exit_err("setup start millis");
	if (setup_winch() == -1)
		exit_err("setup sigwinch");
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
	log_time(ofile, 0);
	for (;;) {
		result = poll(fds, 2, -1);
		if (result == -1) {
			if (winch) {
				if (winchpt(master) == -1)
					exit_err("Update pt win size");
				winch = 0;
			} else {
				exit_err("poll");
			}
		}
		if (fds[FD_PT].revents & POLLIN) {
			if ((nread = read(master, buf, BUFSZ)) > 0) {
				write(STDOUT_FILENO, buf, nread);
				timestamp(ofile);
				nread = escape(buf, nread, escbuf, 2 * BUFSZ);
				write(ofile, buf, nread);
			}
		} else if (fds[FD_IO].revents & POLLIN) {
			if ((nread = read(STDIN_FILENO, buf, BUFSZ)) > 0)
				write(master, buf, nread);
		} else if (fds[FD_IO].revents & POLLHUP) {
			exit_err("STDIN HUP");
		} else if (fds[FD_PT].revents & POLLHUP) {
			log_time(ofile, 1);
			exit(EXIT_SUCCESS);
		}
	}
#undef FD_PT
#undef FD_IO
}
