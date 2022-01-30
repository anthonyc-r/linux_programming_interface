#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>

#define MY_PASSMAX 128
static char buf[MY_PASSMAX];

char *my_getpass(const char *prompt) {
	struct termios old, new;
	int tty;
	ssize_t nread;

	if ((tty = open("/dev/tty", O_RDWR)) == -1)
	       return NULL;
	if (tcgetattr(tty, &old) == -1)
		return NULL;
	new = old;
	new.c_lflag ^= ECHO;
	if (tcsetattr(tty, TCSAFLUSH, &new) == -1)
		return NULL;
	dprintf(tty, "%s: ", prompt);
	if ((nread = read(tty, buf, MY_PASSMAX)) >= MY_PASSMAX)
		return NULL;
	buf[nread] = '\0';
	tcsetattr(tty, TCSAFLUSH, &old);
	dprintf(tty, "\n");
	close(tty);
	return buf;
}

int main(int argc, char **argv) {
	char *pass;
	char test;

	puts("getpass test");
	pass = my_getpass("Enter password(will echo later!)");
	if (pass != NULL) {
		printf("Got password: %s\n", pass);
	} else {
		puts("got null...");
	}
	puts("test echo: ");
	read(STDIN_FILENO, &test, 1);
	exit(EXIT_SUCCESS);
}
