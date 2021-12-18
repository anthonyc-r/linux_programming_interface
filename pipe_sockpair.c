/**
 * 'pipe' implemented using 'socketpair'
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#define BUFSZ 50

int pipe_sockpair(int fds[2]) {
	int socks[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == -1)
		return -1;
	if (shutdown(socks[0], SHUT_RD) == -1)
		return -1;
	if (shutdown(socks[1], SHUT_WR) == -1)
		return -1;
	fds[0] = socks[0];
	fds[1] = socks[1];
	return 0;
}

int main(int argc, char **argv) {
	int fds[2];
	char buf[BUFSZ];
	ssize_t nread;

	if (pipe_sockpair(fds) == -1) {
		perror("pipe_sockpair");
		exit(EXIT_FAILURE);
	}
	switch (fork()) {
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
	case 0:
		close(fds[0]);
		while ((nread = read(fds[1], buf, BUFSZ)) > 0)
			write(STDOUT_FILENO, buf, nread);
		if (nread == -1) {
			perror("read child");
			exit(EXIT_FAILURE);
		}
		break;
	default:
		close(fds[1]);
		while ((nread = read(STDIN_FILENO, buf, BUFSZ)) > 0)
			write(fds[0], buf, nread);
		if (nread == -1) {
			perror("read parent");
			exit(EXIT_FAILURE);
		}
		break;
	}

	exit(EXIT_SUCCESS);
}
