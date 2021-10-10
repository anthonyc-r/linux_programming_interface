/**
 * Demonstrating how unix domain datagram sockets are handled reliably
 * and attempting to send more messages than are being read eventually
 * blocks until messages are read.
 *
 * Note that linux, upon blocking, seems to wait until all messages are
 * read before unblocking the sending process...
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SERVER_PATH "/tmp/test_server.sock"
#define CLIENT_PATH "/tmp/test_client.sock"
#define BUFSZ 50

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void run_client() {
	int cfd, msz, count;
	struct sockaddr_un addr;
	struct sockaddr_un dest;
	char buf[BUFSZ];
	
	memset(&dest, 0, sizeof (struct sockaddr_un));
	memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	dest.sun_family = AF_UNIX;
	strcpy(addr.sun_path, CLIENT_PATH);
	strcpy(dest.sun_path, SERVER_PATH);
	if ((cfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		exit_err("(client) socket");
	if (bind(cfd, (struct sockaddr *)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("(client) bind");
	count = 0;
	while (1) {
		printf("(client) Sending message (%d)...\n", count);
		snprintf(buf, BUFSZ, "%d", count);
		sendto(cfd, buf, BUFSZ, 0, (struct sockaddr *)&dest, sizeof (struct sockaddr_un));
		count++;
	}
	puts("(client) done");
}

static void run_server() {
	int sfd, msz, i;
	struct sockaddr_un addr;
	char buf[BUFSZ];

	memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SERVER_PATH);
	if ((sfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		exit_err("(server) socket");
	if (bind(sfd, (struct sockaddr *)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("(server) bind");
	while (1) {
		sleep(5);
		for (i = 0; i < 100; i++) {
			recv(sfd, buf, BUFSZ, 0);
			printf("(server) got message: %s\n", buf);
		}
	}
}

int main(int argc, char **argv) {
	unlink(SERVER_PATH);
	unlink(CLIENT_PATH);	
	switch (fork()) {
	case -1:
		exit(EXIT_FAILURE);
	case 0:
		run_client();
		break;
	default:
		run_server();
		break;
	}
	exit(EXIT_SUCCESS);
}
