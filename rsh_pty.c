 
/**
 * Based on rsh.c. Allows remotely logging into the system, using a psudoterminal.
 * Usage: ./rsh (server|client host port)
 * Note that this program is quite insecure!
 *
 * Modified to use a pseudoterminal instead of execing sh.
 */
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <poll.h>

#define BUFSZ 100

static void exit_err(char *reason) {
	perror("socket");
	exit(EXIT_FAILURE);
}

static void pass_through(int a, int b) {
	ssize_t nread;
	char buf[BUFSZ];
	struct pollfd polls[2];

#define FDA 0
#define FDB 1
	polls[FDA].fd = a;
	polls[FDA].events = POLLIN;
	polls[FDB].fd = b;
	polls[FDB].events = POLLIN;
	while ((nread = poll(polls, 2, -1)) > 0) {
		if (polls[FDA].revents & POLLIN == POLLIN) {
			if ((nread = read(a, buf, BUFSZ)) == -1)
				exit_err("read fd");
			if (write(b, buf, nread) != nread)
				exit_err("write master");
		} else if (polls[FDB].revents & POLLIN == POLLIN) {
			if ((nread = read(b, buf, BUFSZ)) == -1)
				exit_err("read master");
			if (write(a, buf, nread) != nread)
				exit_err("write fd");
		}
	}
#undef FDA
#undef FDB
}

static void client(char *host, char *port, char *cmd) {
	int sock;
	size_t nread;
	struct sockaddr_storage addr;
	struct addrinfo *res, *curs, hints;
	char buf[100];

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		exit_err("getaddrinfo");
	for (curs = res; curs != NULL; curs = curs->ai_next) {
		if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
			perror("connect");
			continue;
		} else {
			break;
		}
	}
	if (curs == NULL)
		exit_err("no usable adr found");
	freeaddrinfo(res);
	pass_through(sock, STDOUT_FILENO);
}

static void serve(int fd, int sock) {
	int master, slave, nready;
	char *snam;

	if ((master = posix_openpt(O_RDWR)) == -1)
		exit_err("posix_openpt");
	if (unlockpt(master) == -1)
		exit_err("unlockpt");
	if (grantpt(master) == -1)
		exit_err("granpt");
	if ((snam = ptsname(master)) == NULL)
		exit_err("ptsname");
	if ((slave = open(snam, O_RDWR)) == -1)
		exit_err("open slave");

	switch (fork()) {
	case -1:
		perror("fork");
		break;
	case 0:
		close(sock);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		dup2(slave, STDIN_FILENO);
		execl("/bin/login", "login", (char*)NULL);
		perror("execve");
		exit(EXIT_FAILURE);
	default:
		break;
	}
	close(slave);
	pass_through(fd, master);
	close(fd);
	close(master);
}


static void server() {
	int sock, client;
	struct sockaddr_storage addr;
	socklen_t addrlen;
	char host[100], serv[100], c;
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
		serve(client, sock);
	}
	close(sock);
	closelog();
}

int main(int argc, char **argv) {
	if (argc > 1 && strcmp("server", argv[1]) == 0) {
		server();
	} else if (argc > 3 && strcmp("client", argv[1]) == 0) {
		client(argv[2], argv[3], argv[4]);
	} else {
		puts("Usage: ./rsh (server|client <host> <port>)");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
