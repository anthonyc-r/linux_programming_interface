/**
 * Allow shell commands to be remotely executed via a TCP socket.
 * Usage: ./rsh (server|client host port 'shell command')
 * Note that this program is quite insecure!
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/sendfile.h>

#define CMD_SZ 100


static void client(char *host, char *port, char *cmd) {
	int sock;
	size_t  cmd_sz;
	struct sockaddr_storage addr;
	struct addrinfo *res, *curs, hints;
	char buf[100];

	printf("client: %s\n", cmd);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	if (getaddrinfo(host, port, &hints, &res) != 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}
	for (curs = res; curs != NULL; curs = curs->ai_next) {
		if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
			perror("connect");
			continue;
		} else {
			break;
		}
	}
	if (curs == NULL) {
		perror("Couldn't connect");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);
	cmd_sz = strlen(cmd);
	if (write(sock, cmd, cmd_sz) != cmd_sz) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	shutdown(sock, SHUT_WR);
	sendfile(STDOUT_FILENO, sock, NULL, 1000000);
}

static void execmd(int fd, char *cmd) {
	printf("exec command %s\n", cmd);
	write(fd, "ayylmao", 8);
	close(fd);
}


static void serv() {
	int sock, client;
	struct sockaddr_storage addr;
	socklen_t addrlen;
	char host[100], serv[100], command[CMD_SZ], c;
	// Rely on an ephemeral port automatically being selected to avoid
	// some socket setup code.
	
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	if (listen(sock, 10) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	addrlen = sizeof (addr);
	if (getsockname(sock, (struct sockaddr*)&addr, &addrlen) == -1) {
		perror("getsockname");
		exit(EXIT_FAILURE);
	}
	if (getnameinfo((struct sockaddr*)&addr, addrlen, host, 100, serv,
				100, NI_NUMERICHOST | NI_NUMERICSERV) == -1) {
		perror("getnameinfo");
		exit(EXIT_FAILURE);
	}
	
	printf("Server starting on %s:%s\n", host, serv);
	openlog("rsh", LOG_PERROR, LOG_DAEMON);
	
	while ((client = accept(sock, NULL, NULL)) != -1) {
		puts("Accepting client");
		if (read(client, command, CMD_SZ) == -1) {
			close(client);
			continue;
		}
		if (read(client, &c, 1) != 0) {
			// Command too long.
			close(client);
			continue;
		}
		execmd(client, command);
	}
	close(sock);
	closelog();
}

int main(int argc, char **argv) {
	if (argc > 1 && strcmp("server", argv[1]) == 0) {
		serv();
	} else if (argc > 4 && strcmp("client", argv[1]) == 0) {
		client(argv[2], argv[3], argv[4]);
	} else {
		puts("Usage: ./rsh (server|client <host> <port> 'shell command')");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
