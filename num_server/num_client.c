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
#include "num_server.h"

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	char mpath[PATH_MAX];
	int sfd, mfd;
	struct num_open_req req;
	struct num_response res;
	
	req.pid = getpid();
	req.nreq = 5;
	snprintf(mpath, PATH_MAX, FIFO_CLIENT_TEMPLATE, (long long)getpid());
	mfd = open(mpath, O_RDONLY);
	sfd = open(FIFO_SERVER, O_WRONLY);
	
	if (write(sfd, &req, sizeof (struct num_open_req)) == -1)
		exit_err("write");
	if (read(mfd, &res, sizeof (struct num_response)) == -1)
		exit_err("read");
   

	unlink(mpath);
	exit(EXIT_SUCCESS);
}