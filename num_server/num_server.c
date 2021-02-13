/*
* Serve sequential numbers to clients.
* Client opens FIFO_CLIENT_TEMPLATE with it's pid as a parameter, then writes a num_open_req
* structure to the file FIFO_SERVER. The server responds by writing num_response to
* FIFO_CLIENT_TEMPLATE(pid).
*/

#include "num_server.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define REQ_SZ sizeof (struct num_open_req)

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
	int n, i;
	int sfifo, cfifo, sfifo_write;
	char cpath[PATH_MAX];
	ssize_t nread, res_sz;
	struct num_open_req req;
	struct num_response *res;

	if (mkfifo(FIFO_SERVER, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH) == -1)
		exit_err("mkfifo");
	if ((sfifo = open(FIFO_SERVER, O_RDONLY)) == -1)
		exit_err("open 1");
	
	// Avoid ever getting EOF from read(sfifo)
	if ((sfifo_write = open(FIFO_SERVER, O_WRONLY)) == -1)
		exit_err("open 2");

	errno = 0;
	n = 0;
	while ((nread = read(sfifo, &req, REQ_SZ)) == REQ_SZ) {
		printf("Got request - pid: %lld, nreq: %d\n", (long long)req.pid, req.nreq);
		res_sz = (sizeof (struct num_response)) + req.nreq * sizeof (int);
		if ((res = malloc(res_sz)) == NULL)
			exit_err("malloc 1");
		res->nres = req.nreq;
		for (i = 0; i < req.nreq; i++) {
			res->nums[i] = n++;
		}
		if (snprintf(cpath, PATH_MAX, FIFO_CLIENT_TEMPLATE, (long long)req.pid) < 0)
			exit_err("snprintf");
		if ((cfifo = open(cpath, O_WRONLY)) == -1)
			exit_err("open 3");
		if (write(cfifo, &res_sz, sizeof (int)) != sizeof  (int))
			exit_err("write 1");
		if (write(cfifo, res, res_sz) != res_sz)
			exit_err("write 2");
		free(res);
		if (close(cfifo) == -1)
			exit_err("close 1");
	}
	if (nread != 0)
		exit_err("read 1");

	close(sfifo_write);
	close(sfifo);
	unlink(FIFO_CLIENT_TEMPLATE);
	puts("fifo EOF");
	exit(EXIT_SUCCESS);
}
