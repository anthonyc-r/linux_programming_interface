/**
 * Using multiple sockets in order to facilitate high priority data
 * as an alternative to using TCP out of band data.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>

#define BUFSZ 100
#define NAMESZ 50
#define PORTSZ 10
#define SMALL_BUFSZ 10
#ifndef MIN
#define MIN(A,B) (A < B ? A : B)
#define MAX(A,B) (A < B ? B : A)
#endif

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static int get_sock(char *addr, char *port) {
	struct addrinfo hints, *res, *addrp;
	int sock, ires, optval;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (addr == NULL)
		hints.ai_flags = AI_PASSIVE;
	else
		hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if ((ires = getaddrinfo(addr, port, &hints, &res)) != 0) {
		printf("ires: %d\n", ires);
		exit_err("getaddrinfo");
	}
	for (addrp = res; addrp != NULL; addrp = addrp->ai_next) {
		sock = socket(addrp->ai_family, addrp->ai_socktype, 
				addrp->ai_protocol);
		if (sock > -1) {
			if (addr == NULL) {
				optval = 1;
				setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
						&optval, sizeof (optval));
				ires = bind(sock, addrp->ai_addr, addrp->ai_addrlen);
			} else {
				ires = connect(sock, addrp->ai_addr, addrp->ai_addrlen);
			}
			if (ires == 0)
				break;
			close(sock);
			sock = -1;
		}
	}
	freeaddrinfo(res);
	if (addrp == NULL) {
		if (sock > -1)
			close(sock);
		return -1;
	}
	return sock;
}

static void handle_client(int client[2]) {
	fd_set rset, wset, eset;
	char buf[SMALL_BUFSZ];
	int nfds;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET(client[0], &rset);
	FD_SET(client[1], &rset);
	nfds = MAX(client[0], client[1]) + 1;
	while (pselect(nfds, &rset, &wset, &eset, NULL, NULL) != -1) {
		if (FD_ISSET(client[1], &rset)) {
			if (read(client[1], buf, SMALL_BUFSZ) < 1)
				break;
			buf[SMALL_BUFSZ - 1] = '\0';
			printf("!!PRIO!! - %s\n", buf);
		} else if (FD_ISSET(client[0], &rset)) {
			if (read(client[0], buf, SMALL_BUFSZ) < 1)
				break;
			buf[SMALL_BUFSZ - 1] = '\0';
			printf("%s\n");
		}
		FD_ZERO(&rset);
		FD_SET(client[0], &rset);
		FD_SET(client[1], &rset);
	}
}

static void server(char *port) {
	struct sockaddr_storage caddr;
	socklen_t clen;
	int client[2], sock, ires;
	char buf[BUFSZ], cname[NAMESZ], cport[PORTSZ];
	ssize_t nread;

	if ((sock = get_sock(NULL, port)) == -1)
		exit_err("Failed to get server sock");
	if (listen(sock, 10) == -1)
		exit_err("listen");
	clen = sizeof (caddr);
	while ((client[0] = accept(sock, (struct sockaddr*)&caddr, &clen)) > -1) {
		if ((nread = read(client[0], buf, BUFSZ)) < 1) {
			printf("Failed to read prio port: %s\n", strerror(errno));
			continue;
		}
		printf("Opening priorty connection on port %s\n", buf);
		ires = getnameinfo((struct sockaddr*)&caddr, clen, cname, 
				NAMESZ, cport, PORTSZ, NI_NUMERICHOST | 
				NI_NUMERICSERV);
		if (ires != 0) {
			puts(gai_strerror(ires));
			perror("Failed to get client nameinfo");
			continue;
		}
		if ((client[1] = get_sock(cname, buf)) == -1) {
			perror("Failed to connect to prio sock");
			continue;
		}
		puts("Ready to handle client!");
		handle_client(client);
		close(client[0]);
		close(client[1]);
	}
}

static void client(char *addr, char *port) {
	int sock, prio, ires, priofd;
	char pport[PORTSZ];
	char buf[BUFSZ];
	struct sockaddr_storage sock_addr;
	socklen_t alen;

	if ((prio = get_sock(NULL, "0")) == -1)
		exit_err("Failed to get priority sock");
	if (listen(prio, 10) == -1)
		exit_err("failed to listen on prio sock");
	if ((sock = get_sock(addr, port)) == -1)
		exit_err("Failed to get normal sock");
	alen = sizeof (sock_addr);
	ires = getsockname(prio, (struct sockaddr*)&sock_addr, &alen);
	if (ires == -1)
		exit_err("Couldn't get prio sock addr");
	ires = getnameinfo((struct sockaddr*)&sock_addr, alen, NULL, 0, 
			pport, PORTSZ, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ires != 0) {
		puts(gai_strerror(ires));
		exit_err("Couldn't get prio name info");
	}
	printf("Writing port number %s to server\n", pport);
	write(sock, pport, MIN(strlen(pport), PORTSZ));
	priofd = accept(prio, NULL, NULL);
	if (priofd == -1)
		exit_err("failed to accept priority connection from serv");
	puts("established prio connection");
	for (;;) {
		read(STDIN_FILENO, buf, BUFSZ);
		// If user input starts with ! send it via the priority channel
		if (buf[0] == '!')
			write(priofd, buf, BUFSZ);
		else
			write(sock, buf, BUFSZ);
	}
}

int main(int argc, char **argv) {
	if (argc > 3 && strcmp("client", argv[1]) == 0) {
		client(argv[2], argv[3]);
	} else if (argc > 2 && strcmp("server", argv[1]) == 0) {
		server(argv[2]);
	} else {
		puts("Usage: ./inet_prio (client <addr> <port>|server <port>)");
	}
	exit(EXIT_SUCCESS);
}
