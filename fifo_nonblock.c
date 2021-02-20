/*
* This program verfies the behavior of the O_NONBLOCK flag when used on FIFOs.
* Tested under OpenBSD.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/fifononblock"

static void exit_err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int fd, fd2, sig, waitstat, i;
	struct stat sb;
	sigset_t mask;
	pid_t child;
	char buf[2 * PIPE_BUF];
	
	// Prep for sync later on.
	errno = 0;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	if (errno != 0)
		exit_err("sig prep");
	
	if (stat(FIFO_PATH, &sb) != -1)
		unlink(FIFO_PATH);
	if (mkfifo(FIFO_PATH, S_IRWXU) == -1)
		exit_err("mkfifo");
	puts("Open FIFO with O_NONBLOCK for writing");
	if ((fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK)) == -1)
		printf("Result: %s\n", strerror(errno));
	else
		puts("Result: Success!");
	close(fd);
	puts("");
	
	puts("Open FIFO with O_NONBLOCK for reading");
	if ((fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) == -1)
		printf("Result: %s\n", strerror(errno));
	else
		puts("Result: Success!");
	close(fd);
	puts("");
	
	// Set up an open fifo for the next set of tests.
	switch ((child = fork())) {
	case -1:
		exit_err("fork");
	case 0:
		errno = 0;
		fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
		fd2 = open(FIFO_PATH, O_WRONLY);
		puts("Child opened FIFO for read&write.");
		kill(getppid(), SIGUSR1);
		if (errno != 0) {
			puts("Child error");
			_exit(EXIT_FAILURE);
		}
		sigwait(&mask, &sig);
		puts("Tearing down child");
		close(fd);
		close(fd2);
		_exit(EXIT_SUCCESS);
	default:
		break;
	}
	// Wait for child to finish.
	if (sigwait(&mask, &sig) != 0)
		exit_err("sigsuspend");
	
	
	puts("Attempting to read an open O_NONBLOCK FIFO with no data pending");
	if ((fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) == -1)
		exit_err("open");
	if (read(fd, buf, 10) == -1)
		printf("Result: ERROR! - %s\n", strerror(errno));
	else 
		puts("Result: SUCCESS!");
	close(fd);
	puts("");
		
	puts("Attempting to write to an open O_NONBLOCK FIFO with a full buffer");
	printf("Value of PIPE_BUF is %d\n", PIPE_BUF);
	if ((fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK | O_SYNC)) == -1)
		exit_err("open");
	for (i = 0; i < 20; i++) {
		printf("Writing %d bytes\n", PIPE_BUF);
		if (write(fd, buf, PIPE_BUF) == -1) {
			printf("Result: ERROR! - %s\n", strerror(errno));
			break;
		} else {
			puts("Result: SUCCESS!");
		}
	}
	close(fd);
	puts("");
	
	kill(child, SIGUSR1);
	wait(&waitstat);
	puts("Exit");
	exit(EXIT_SUCCESS);
}
