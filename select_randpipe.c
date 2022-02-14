/**
 * Example usage of 'select()'
 * Create a number of pipes, write bytes to the write end, then 
 * select to find out which pipes have data to be read.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <fcntl.h>

#define MAXP 100

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int rfd, npipes, nwrite, i, j, target, ok;
	int pipes[MAXP][2], written[MAXP];
	char *ep;
	unsigned char r;
	fd_set rset;

	if (argc < 3)
		exit_err("usage: ./select_randpipe npipes nwritten");
	npipes = (int)strtol(argv[1], &ep, 10);
	if (*ep != '\0')
		exit_err("usage: ./select_randpipe npipes nwritten");
	if (npipes >= MAXP)
		exit_err("npipes is too big");
	nwrite = (int)strtol(argv[2], &ep, 10);
	if (*ep != '\0')
		exit_err("usage: ./select_randpipe npipes nwritten");
	if ((rfd = open("/dev/random", O_RDONLY)) == -1)
		exit_err("open /dev/random");
	for (i = 0; i < npipes; i++) {
		if (pipe(pipes[i]) == -1)
			exit_err("pipe");
	}
	for (i = 0; i < nwrite; i++) {
		do {
			read(rfd, &r, 1);
			target = (int)(((double)r / 256.0) * 
					(double)nwrite);
			ok = 1;
			for (j = 0; j < i; j++) {
				if (written[j] == target) {
					ok = 0;
					break;
				}
			}
		} while (!ok);
		written[i] = target;
		write(pipes[target][1], &r, 1);
	}
	FD_ZERO(&rset);
	for (i = 0; i < npipes; i++) {
		FD_SET(pipes[i][0], &rset);
	}
	// Assume pipe[1] is the greater fd - is it?
	i = select(pipes[npipes - 1][1] + 1, &rset, NULL, NULL, NULL);
	printf("%d fds ready\n", i);
	for (i = 0; i < npipes; i++) {
		if (FD_ISSET(pipes[i][0], &rset) != 0)
			printf("fd %d is ready to read!\n", pipes[i][0]);
	}
	
	exit(EXIT_SUCCESS);
}
