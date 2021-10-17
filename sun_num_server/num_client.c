/*
* An example client program communicating with the num_server.
*/

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "num_server.h"

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int sfd, i;
	struct sockaddr_un addr;
	struct num_greeting req;
	struct num_response res;

	memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(&addr.sun_path[1], SERVER_NAME, 80);
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	if (connect(sfd, (struct sockaddr*)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("connect");

	req.nreq = 5;
	if (argc > 1)
		sleep(2);
	if (write(sfd, &req, GREETSZ) != GREETSZ)
		exit_err("Write greeting");
	if (read(sfd, &res, RESSZ) != RESSZ)
		exit_err("Read response");
	close(sfd);

	printf("Got response!");
	for (i = 0; res.nums[i] != -1; i++) {
		printf("%d, ", res.nums[i]);
	}
	puts("");
	exit(EXIT_SUCCESS);
}
