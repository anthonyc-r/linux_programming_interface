/**
 * Replayable implementation of script(1).
 * Runs $SHELL under a pseudoterminal slave, the master then outputs
 * all input (where echoed) and output to the specified file. Each output is timestamped
 * so that it can be replayed.
 * For the sake of simplicity, this doesn't output the human readable <ts> <space> <line> <nl>
 * format suggested in the exercise...
 * Usage:
 * Record a shell session: ./script <output_file>
 * Replay a previously recorded session: ./script <previously_output_file> replay
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
static unsigned long long get_elapsed(unsigned long long since) {
	struct timespec ts;
	unsigned long long current;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
		return -1;
	current = (unsigned long long)ts.tv_sec * 1000L;
	current += (unsigned long long)(ts.tv_nsec / 1000000L);
	return current - since;
}
static int timestamp(int fd) {
	unsigned long long millis, diff;
	int nprint;

	if (get_millis(&millis) == -1)
		return -1;
	if (millis < smillis)
		return -1;
	diff = millis - smillis;
	write(fd, &diff, sizeof (diff));
	return 0;
}

static void record(int ofile) {
	struct pollfd fds[2];
	char buf[BUFSZ], *sname;
	ssize_t nread;
	int result, master;

	if (setup_winch() == -1)
		exit_err("setup sigwinch");
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
	setraw();

#define FD_PT 0
#define FD_IO 1
	fds[FD_PT].fd = master;
	fds[FD_PT].events = POLLIN;
	fds[FD_IO].fd = STDIN_FILENO;
	fds[FD_IO].events = POLLIN;
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
				write(ofile, &nread, sizeof (nread));
				write(ofile, buf, nread);
			}
		} else if (fds[FD_IO].revents & POLLIN) {
			if ((nread = read(STDIN_FILENO, buf, BUFSZ)) > 0)
				write(master, buf, nread);
		} else if (fds[FD_IO].revents & POLLHUP) {
			exit_err("STDIN HUP");
		} else if (fds[FD_PT].revents & POLLHUP) {
			exit(EXIT_SUCCESS);
		}
	}
#undef FD_PT
#undef FD_IO
}

static void replay(int ifile) {
	ssize_t nread;
	ssize_t len;
	char buf[BUFSZ];
	unsigned long long millis, elapsed;

	while ((nread = read(ifile, &millis, sizeof (millis))) > 0) {
		if ((nread = read(ifile, &len, sizeof (len))) == -1)
			break;
		if ((nread = read(ifile, buf, len)) == -1)
			break;
		for (;;) {
			elapsed = get_elapsed(smillis);
			if (elapsed < millis) {
				usleep(50000);
				continue;
			}
			write(STDOUT_FILENO, buf, nread);
			break;
		}
	}
	if (nread == -1)
		exit_err("read ifile");
}

int main(int argc, char **argv) {
	int file, rplay;

	if (argc < 2)
		exit_err("usage: ./script <output_file> | <input_file> replay");
	rplay = argc > 2;

	if (get_millis(&smillis) == -1)
		exit_err("setup start millis");
	if ((file = open(argv[1], O_RDWR | O_CREAT, S_IRWXU)) == -1)
		exit_err("failed to open output file");

	if (rplay) {
		replay(file);
	} else {
		record(file);
	}
}
