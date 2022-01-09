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

#define BUFSZ 100
#define NAMESZ 50
#define PORTSZ 10
#ifndef MIN
#define MIN(A,B) (A < B ? A : B)
#endif

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static int get_sock(char *addr, char *port) {
	struct addrinfo hints, *res, *addrp;
	int sock, ires;

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
		if (ires == -1) {
			puts("Failed to get client nameinfo");
			continue;
		}
		if ((client[1] = get_sock(cname, cport)) == -1) {
			puts("Failed to connect to prio sock");
			continue;
		}
		puts("Ready to handle client!");
		handle_client(client);
		close(client[0]);
		close(client[1]);
	}
}

static void client(char *addr, char *port) {
	int sock, prio, ires;
	char pport[PORTSZ];
	struct sockaddr_storage sock_addr;
	socklen_t alen;

	if ((prio = get_sock(NULL, "0")) == -1)
		exit_err("Failed to get priority sock");
	if ((sock = get_sock(addr, port)) == -1)
		exit_err("Failed to get normal sock");
	ires = getsockname(prio, (struct sockaddr*)&sock_addr, &alen);
	if (ires == -1)
		exit_err("Couldn't get prio sock addr");
	ires = getnameinfo((struct sockaddr*)&sock_addr, alen, NULL, 0, 
			pport, PORTSZ, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ires == -1)
		exit_err("Couldn't get prio name info");
	write(sock, pport, MIN(strlen(pport), PORTSZ));
	sleep(10);
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
