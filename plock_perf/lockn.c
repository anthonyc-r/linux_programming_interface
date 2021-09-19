/**
 * ./timed_lockn <path> <n>
 * Attempt to lock the n*2 th byte of the provided file,
 * timing how long the operation takes.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define NTIMES 10000

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	struct flock fl;
	int fd, i;
	long n;
	char *ep;

	n = strtol(argv[2], &ep, 10);
	if (ep == NULL || *ep != '\0')
		exit_err("strtol");

	if ((fd = open(argv[1], O_RDWR)) == -1)
		exit_err("open");

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = n * 2;
	fl.l_len = 1;
	printf("Will attempt to lock byte %ld, on file %s\n", n * 2, argv[1]);

	for (i = 0; i < NTIMES; i++) {
		if (fcntl(fd, F_SETLK, &fl) != -1 || errno != EAGAIN)
			exit_err("Expected EAGAIN error on lock");
	}
	
	exit(EXIT_SUCCESS);
}
