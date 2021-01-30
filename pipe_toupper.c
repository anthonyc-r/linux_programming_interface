/**
* This program forks a child which acts as a server, which converts text to upper case.
* Communication between parent and child process is done via pipes.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define BUF_SZ 100
static void _exit_err(char *reason) {
	printf("error %s (%s)\n", reason, strerror(errno));
	_exit(EXIT_FAILURE);
}
static void exit_err(char *reason) {
	printf("error %s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void runsvr() {
	char buf[BUF_SZ];
	ssize_t nread, i;

	while ((nread = read(STDIN_FILENO, buf, BUF_SZ)) > 0) {
		for (i = 0; i < nread; i++) {
			buf[i] = (char)toupper((int)buf[i]);
		}
		if (write(STDOUT_FILENO, buf, nread) != nread)
			_exit_err("write");
	}
	if (nread == -1)
		_exit_err("read");
	puts("Child exiting...");
	_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	char buf[BUF_SZ];
	ssize_t nread;
	int stc_fd[2];
	int cts_fd[2];
	
	if (pipe(cts_fd) == -1)
		exit_err("pipe1");
	if (pipe(stc_fd) == -1)
		exit_err("pipe2");
	switch (fork()) {
	case -1:
		exit_err("fork");
	case 0:
		break;
	default:
		// As below
		if (close(stc_fd[1]) == -1)
			exit_err("close1");
		if (close(cts_fd[0]) == -1)
			exit_err("close2");

		// An excuse to use dup2 lol.
		// For the sake of good habit, check fds arent't the same before using dup2
		if (stc_fd[0] != STDOUT_FILENO) {
			if (dup2(stc_fd[0], STDOUT_FILENO) == -1)
				exit_err("dup2 1");
		}
		if (cts_fd[1] != STDIN_FILENO) {
			if (dup2(cts_fd[1], STDIN_FILENO) == -1)
				exit_err("dup2 2");
		}
		runsvr();
	}
	
	// Ensure unused fds are closed to ensure the other end gets
	// the appropriate error when we have stopped listening.
	if (close(stc_fd[0]) == -1)
		exit_err("close3");
	if (close(cts_fd[1]) == -1)
		exit_err("close3");
	while ((nread = read(STDIN_FILENO, buf, BUF_SZ)) > 0) {
		puts("");
		if (write(cts_fd[0], buf, nread) == -1)
			exit_err("write1");
		if (read(stc_fd[1], buf, nread) == -1)
			exit_err("read1");
		if (write(STDOUT_FILENO, buf, nread) != nread)
			exit_err("write2");
		puts("");
	}
	if (nread == -1)
		exit_err("read2");
	puts("Exit parent");
	exit(EXIT_SUCCESS);
}