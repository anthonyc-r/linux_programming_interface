/**
 * nlock.c
 * Usage: ./nlock <path> <n>
 * Place n locks on file at path, each lock on alternating bytes of the file.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int fd, i, n;
	char *ep;
	struct flock fl;

	n = (int)strtol(argv[2], &ep, 10);
	if (ep == NULL || *ep != '\0')
		exit_err("strtol");

	if ((fd = open(argv[1], O_RDWR)) == -1)
		exit_err("open");
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;

	puts("Locking...");
	for (i = 0; i < n; i++) {
		fl.l_start = i * 2;
		if (fcntl(fd, F_SETLK, &fl) == -1)
			exit_err("fcntl F_SETLK");
	}
	puts("Done!");
	pause();
	exit(EXIT_SUCCESS);
}
