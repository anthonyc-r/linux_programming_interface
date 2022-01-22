#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>

int my_isatty(int fd) {
	struct termios term;

	return tcgetattr(fd, &term) + 1;
}

int main(int argc, char **argv) {
	int fd;

	printf("isatty(STDIN_FILENO): %d\n", isatty(STDIN_FILENO));
	printf("isatty(STDOUT_FILENO): %d\n", isatty(STDOUT_FILENO));

	fd = open("/dev/random", O_RDONLY);
	printf("fd = /dev/random, isatty(fd): %d\n", isatty(fd));
	exit(EXIT_SUCCESS);
}