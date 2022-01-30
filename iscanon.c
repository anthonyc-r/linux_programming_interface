/**
 * Display whether terminal is in canonical or non-canonical mode
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>


static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}
int main(int argc, char **argv) {
	struct termios tio;

	if (tcgetattr(STDIN_FILENO, &tio) == -1)
		exit_err("tcgetattr");
	if (tio.c_lflag & ICANON == ICANON) {
		puts("Terminal in canonical mode");
	} else {
		puts("Terminal in non-canonical mode");
		printf("TIME: %d\n", (int)tio.c_cc[VTIME]);
		printf("MIN: %d\n", (int)tio.c_cc[VMIN]);
	}

	exit(EXIT_SUCCESS);
}
