/**
 * Basic implementation of script(1).
 * Runs $SHELL under a pseudoterminal slave, the master then outputs
 * all input (where echoed) and output to the specified file.
 * This updated version uses cat and tee programs, instead of
 * handling IO redirection ourselves via select.
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
#include <errno.h>
#include <sys/wait.h>

#define BUFSZ 100

static struct termios tios_restore;
static volatile sig_atomic_t winch;

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

int main(int argc, char **argv) {
	int master, ofile, result, wstat;
	char *sname, buf[BUFSZ];
	struct pollfd fds[2];
	struct termios raw;
	ssize_t nread;
	pid_t tee, cat, wpid;

	if (argc < 2)
		exit_err("usage: ./script <output_file>");
	if (setup_winch() == -1)
		exit_err("setup sigwinch");
	if ((ofile = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU)) == -1)
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

	log_time(ofile, 0);
	fsync(ofile);
	switch ((tee = fork())) {
		case -1:
			exit_err("fork");
		case 0:
			dup2(master, STDIN_FILENO);
			execl("/bin/tee", "tee", "-a", argv[1], (char*)NULL);
			exit_err("tee");
		default:
			break;
	}
	switch ((cat = fork())) {
		case -1:
			exit_err("cat");
		case 0:
			dup2(master, STDOUT_FILENO);
			execl("/bin/cat", "cat", (char*)NULL);
			exit_err("cat");
		default:
			break;
	}
	// Monitor our child procs
	for (;;) {
		if ((wpid = wait(&wstat)) == -1) {
			switch (errno) {
				case ECHILD:
					fsync(ofile);
					log_time(ofile, 1);
					exit(EXIT_SUCCESS);
				case EINTR:
					continue;
				default:
					kill(cat, SIGINT);
					kill(tee, SIGINT);
					exit_err("wait");
			}
		// Tee should be the first to quit, but just in case.
		} else if (wpid == cat) {
			kill(tee, SIGINT);
		} else if (wpid == tee) {
			kill(cat, SIGINT);
		}
	}
}
