/*
 * Number server using internet stream sockets
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include "readlinebuf.c"

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int fd, sock, number, req, i;
	struct addrinfo *nfo, *nfo_res, hints;
	char hostnam[100], servnam[100], buf[100], *ep;
	ssize_t nread;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;	
	if (getaddrinfo(NULL, argv[1], &hints, &nfo_res) == -1)
		exit_err("getaddrinfo");
	for(nfo = nfo_res; nfo != NULL; nfo = nfo->ai_next) {
		if (bind(sock, nfo->ai_addr, nfo->ai_addrlen) == 0) {
			puts("Bind success");
			break;
		}
	}
	if (nfo == NULL)
		exit_err("Couldn't find an address to bind to");
	if (getnameinfo(nfo->ai_addr, nfo->ai_addrlen, hostnam, 100, servnam, 100, NI_NUMERICHOST | NI_NUMERICSERV) == -1)
		exit_err("getnameinfo");
	if (listen(sock, 512) == -1)
		exit_err("listen");
	printf("Now listening on %s:%s\n", hostnam, servnam);
	freeaddrinfo(nfo_res);
	
	number = 0;
	while ((fd = accept(sock, NULL, NULL)) != -1) {
		puts("Accepted connection");
		if ((nread = read(fd, buf, 100)) == -1)
			exit_err("rec");
		req = (int)strtol(buf, &ep, 10);
		if (ep == buf) {
			puts("Client request error. Closing");
			close(fd);
			continue;
		}
		for (i = 0; i < req; i++) {
			// Will never reach 99 digits from formatting an int
			// so don't worry about truncation...
			if ((nread = snprintf(buf, 100, "%d\n", number++)) > 0)
				write(fd, buf, nread + 1);
		}
		close(fd);
	}
	exit_err("read");
}
