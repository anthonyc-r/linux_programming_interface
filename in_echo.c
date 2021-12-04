/**
 * An internet domain stream socket echo server with a thread limit and
 * inetd lanuch option -i
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>

#define BUFSZ 100
#define THREADLIM 100

void *echo(void *arg) {
	char buf[BUFSZ];
	int fd, nread;

	fd = (int)arg;
	while ((nread = read(fd, buf, BUFSZ)) > 0 || errno == EINTR) {
		if (write(fd, buf, nread) != nread) {
			syslog(LOG_ERR, "Failed to complete write to client");
			errno = 0;
			break;
		}
	}
	if (errno != 0)
		syslog(LOG_ERR, "Error servicing client: %s", strerror(errno));
}

int main(int argc, char **argv) {
	openlog("in_echo", 0, LOG_DAEMON);
	if (argc > 1 && strcmp(argv[1], "-i") == 0) {
		echo((void*)STDOUT_FILENO);
	} else {
	
	}
	closelog();
	exit(EXIT_SUCCESS);
}
