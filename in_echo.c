/**
 * An internet domain stream socket echo server with a thread limit and
 * inetd lanuch option -i
 *
 * Note that non privilaged programs cannot bind to the echo port.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFSZ 100
#define THREADLIM 100
#define BKLOG 20

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

int mksock() {
	struct addrinfo hints, *res, *cur;
	int sock;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	if (getaddrinfo(NULL, "echo", &hints, &res) != 0)
		return -1;
	sock = -1;
	for (cur = res; cur != NULL; cur = cur->ai_next) {
		if ((sock = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) == -1)
			continue;
		if (bind(sock, cur->ai_addr, cur->ai_addrlen) == -1) {
			close(sock);
			sock = -1;
			continue;
		}
		if (listen(sock, BKLOG) == -1) {
			close(sock);
			sock = -1;
		}
		break;

	}
	return sock;

}

void run_server() {
	pthread_t thread;
	int sock, fd;
	syslog(LOG_INFO, "running");

	if (daemon(0, 0) == -1) {
		syslog(LOG_ERR, "daemon() failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO,"detached");
	if ((sock = mksock()) == -1) {
		syslog(LOG_ERR, "Failed to create listeneing socket");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "listening socket created");
	while ((fd = accept(sock, NULL, NULL)) > -1) {
		syslog(LOG_INFO, "accepted connection...");
		if (pthread_create(&thread, NULL, echo, (void*)fd) == 0) {
			syslog(LOG_ERR, "Failed to create thread");
			continue;
		}
		syslog(LOG_INFO, "detatched thread to handle child");
		if(pthread_detach(thread) == -1) {
			syslog(LOG_ERR, "Failed to detach thread, exiting");
			exit(EXIT_FAILURE);
		}
	}
	
}

int main(int argc, char **argv) {
	openlog("in_echo", 0, LOG_DAEMON);
	if (argc > 1 && strcmp(argv[1], "-i") == 0) {
		echo((void*)STDOUT_FILENO);
	} else {
		run_server();
	}
	closelog();
	exit(EXIT_SUCCESS);
}
