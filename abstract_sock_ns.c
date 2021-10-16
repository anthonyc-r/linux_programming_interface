/**
 * Using the linux-specific 'abstract socket namespace' for unix domain
 * sockets.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define CLIENT_NAME "no a file path"
#define SERVER_NAME "also not a file path"
#define BUFSZ 50

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void server() {
	struct sockaddr_un addr, caddr;
	int sfd, nread, cfd, i;
	char buf[BUFSZ];
	socklen_t len;

	memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(&addr.sun_path[1], SERVER_NAME);
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	if (bind(sfd, (struct sockaddr*)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("bind");
	if (listen(sfd, 10) == -1)
		exit_err("listen");
	if ((cfd = accept(sfd, (struct sockaddr*)&caddr, &len)) == -1)
		exit_err("accept");
	while ((nread = read(cfd, buf, BUFSZ)) > 0) {
		for (i = 0; i < nread; i++) {
			buf[i] = toupper(buf[i]);
		}
		if (write(cfd, buf, nread) != nread)
			exit_err("write");
	}
	if (nread == -1)
		exit_err("read");

}

static void client() {
	struct sockaddr_un addr, saddr;
	int fd, nread;
	char buf[BUFSZ];

	memset(&addr, 0, sizeof (struct sockaddr_un));
	memset(&saddr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(&addr.sun_path[1], CLIENT_NAME);
	saddr.sun_family = AF_UNIX;
	strcpy(&saddr.sun_path[1], SERVER_NAME);
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	if (bind(fd, (struct sockaddr*)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("bind");
	if (connect(fd, (struct sockaddr*)&saddr, sizeof (struct sockaddr_un)) == -1)
		exit_err("connect");
	while ((nread = read(STDIN_FILENO, buf, BUFSZ)) > 0) {
		if (write(fd, buf, nread) != nread)
			exit_err("write to server");
		if ((nread = read(fd, buf, nread)) == -1)
		       exit_err("read from server");
		if (write(STDOUT_FILENO, buf, nread) != nread)
			exit_err("write to stdout");
	}
	if (nread == -1)
		exit_err("read stdin");

}

int main(int argc, char **argv) {
	if (strcmp(argv[1], "server") == 0)
		server();
	else if (strcmp(argv[1], "client") == 0)
		client();
	else
		exit_err("Usage: ./abstract_sock_ns client|server");
	exit(EXIT_SUCCESS);
}
