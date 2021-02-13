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
#include <sys/stat.h>
#include "num_server.h"

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	char mpath[PATH_MAX];
	int sfd, mfd, res_sz, i;
	struct num_open_req req;
	struct num_response *res;
	
	req.pid = getpid();
	req.nreq = 5;
	snprintf(mpath, PATH_MAX, FIFO_CLIENT_TEMPLATE, (long long)getpid());
	// Make our fifo ready for the server to open. Don't open yet though else it'll block.
	if (mkfifo(mpath, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH) == -1)
		exit_err("mkfifo (client)");

	sfd = open(FIFO_SERVER, O_WRONLY);
	if (write(sfd, &req, sizeof (struct num_open_req)) == -1)
		exit_err("write - num_server probably isn't running.");

	// Now the server should proceed to open our fifo for writing.
	mfd = open(mpath, O_RDONLY);
	if ((read(mfd, &res_sz, sizeof (int))) == -1)
		exit_err("read");
	printf("Need to alloc a %d byte struct\n", res_sz);
	if ((res = malloc(res_sz)) == NULL)
		exit_err("malloc");
	if ((read(mfd, res, res_sz)) == -1)
		exit_err("read");
	printf("Got response with sequence length: %d\n", res->nres);
	for (i = 0; i < res->nres; i++) {
		printf("%d, ", res->nums[i]);
	}
	puts("");
	free(res);
	unlink(mpath);
	exit(EXIT_SUCCESS);
}