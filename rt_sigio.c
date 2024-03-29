/**
 * Example of using the linux 'real time signal' feature with
 * signal driven io.
 */

// For F_SETSIG
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define RTSIG ( SIGRTMIN )

static volatile sig_atomic_t signalled;
static struct termios *tios_reset;

static int set_cbrk() {
	struct termios tios;
	if (tcgetattr(STDIN_FILENO, &tios) == -1)
		return -1;
	tios.c_lflag ^= ~(ICANON | ECHO);
	tios.c_lflag |= ISIG;
	tios.c_iflag &= ~ICRNL;
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios) == -1)
		return -1;
	return 0;
}

static void spin() {
	int i;
	float f;

	for(i = 0; i < 100000000; i++)
		f *= 2.1;
}

static int set_nblock(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1)
		return -1;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;
	return 0;
}

static int set_async(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1)
		return -1;
	if (fcntl(fd, F_SETOWN, getpid()) == -1)
		return -1;
	if (fcntl(fd, F_SETFL, flags | O_ASYNC) == -1)
		return -1;
	return 0;
}

static void exit_err(char *reason) {
	perror(reason);
	if (tios_reset != NULL)
		tcsetattr(STDIN_FILENO, TCSAFLUSH, tios_reset);
	exit(EXIT_FAILURE);
}

static void handle(int sig, siginfo_t *info, void *ucontext) {
	char *code;

	signalled = 1;
	switch (info->si_code) {
		case POLL_IN:
			code = "POLL_IN";
			break;
		case POLL_OUT:
			code = "POLL_OUT";
			break;
		case POLL_MSG:
			code = "POLL_MSG";
			break;
		case POLL_ERR:
			code = "POLL_ERR";
			break;
		case POLL_PRI:
			code = "POLL_PRIO";
			break;
		case POLL_HUP:
			code = "POLL_HUP";
			break;
		default:
			code = "UNKNOWN";
	}
	// Not async signal safe...
	printf("si_fd: %d\n", info->si_fd);
	printf("si_code: %s\n", code);
}

static int setup_handler() {
	struct sigaction sigact;
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_sigaction = handle;
	sigact.sa_flags = SA_SIGINFO;
	sigemptyset(&sigact.sa_mask);
	return sigaction(RTSIG, &sigact, NULL);
}

int main(int argc, char **argv) {
	int count;
	char c;
	struct termios old;

	if (tcgetattr(STDIN_FILENO, &old) == -1)
		exit_err("tcgetattr");
	tios_reset = &old;

	if (set_nblock(STDIN_FILENO) == -1)
		exit_err("set stdin nonblock");

	if (setup_handler() == -1)
		exit_err("setup handler");

	// Set this before async to avoid potentially getting a SIGINFO.
	if (fcntl(STDIN_FILENO, F_SETSIG, RTSIG) == -1)
		exit_err("Setup realtime signal");

	if (set_async(STDIN_FILENO) == -1)
		exit_err("set async");

	if (set_cbrk() == -1)
		exit_err("set cbreak mode");

	puts("Press # to exit.");
	for(count = 0, c = ' '; c != '#'; count++) {
		spin();
		if (signalled) {
			signalled = 0;
			while (read(STDIN_FILENO, &c, 1) > 0) {
				printf("INPUT: %c. COUNT: %d\n", c, count);
				if (c == '#')
					break;
			}
		}
	}
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
	exit(EXIT_SUCCESS);
}
