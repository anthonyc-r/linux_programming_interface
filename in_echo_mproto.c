/**
 * This is a simplified ./in_echo.c, but it accepts connections on both TCP and
 * UDP sockets, making use of poll.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#define BUFSZ 100
#define BKLOG 20


static int mksock(int type) {
	struct addrinfo hints, *res, *cur;
	int sock;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = type;
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
		if (type == SOCK_STREAM && listen(sock, BKLOG) == -1) {
			close(sock);
			sock = -1;
		}
		break;

	}
	return sock;
}

static void streamsock(int sock) {
	char buf[BUFSZ];
	ssize_t nread;
	int fd;

	if ((fd = accept(sock, NULL, NULL)) == -1) {
		perror("Failed to accept from stream sock");
		return;
	}
	puts("accepted connection...");
	while ((nread = read(fd, buf, BUFSZ)) > 0) {
		write(fd, buf, nread);
	}
	puts("done with client");
}

static void dgramsock(int sock) {
	struct sockaddr_storage addr;
	char buf[BUFSZ];
	ssize_t nread;
	socklen_t socklen;

	socklen = sizeof (addr);
	nread = recvfrom(sock, buf, BUFSZ, 0, (struct sockaddr*)&addr, 
			&socklen);
	if (nread == -1) {
		perror("recvfrom");
		exit(EXIT_FAILURE);
	}
	if (sendto(sock, buf, nread, 0, (struct sockaddr*)&addr, socklen) 
			!= nread) {

		perror("sendto");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv) {
	int ssock, dsock, fd;
	struct pollfd pfds[2];
	if ((ssock = mksock(SOCK_STREAM)) == -1) {
		perror("Failed to create listeneing tcp socket");
		exit(EXIT_FAILURE);
	}
	if ((dsock = mksock(SOCK_DGRAM)) == -1) {
		perror("Failed to create dgram socket");
		exit(EXIT_FAILURE);
	}
	puts("listening socket created");

#define IDX_STREAM 0
#define IDX_DGRAM 1	
	pfds[IDX_STREAM].fd = ssock;
	pfds[IDX_STREAM].events = POLLIN;
	pfds[IDX_DGRAM].fd = dsock;
	pfds[IDX_DGRAM].events = POLLIN;
	while (poll(pfds, 2, 0) != -1) {
		if (pfds[IDX_STREAM].revents & POLLIN == POLLIN) {
			puts("stream socket ready");
			switch (fork()) {
			case 0:
				streamsock(ssock);
				break;
			case -1:
				perror("fork");
				exit(EXIT_FAILURE);
			default:
				break;
			}
	
		} else if (pfds[IDX_DGRAM].revents & POLLIN == POLLIN) {
			puts("dgram socket ready");
			dgramsock(dsock);
		}
	}
#undef IDX_STREAM
#undef IDX_DGRAM
	exit(EXIT_SUCCESS);
}
