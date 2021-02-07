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

#define REQ_SZ sizeof (struct num_open_req)

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
	int sfifo, cfifo;
	ssize_t nread;
	struct num_open_req req;

	if (mkfifo(FIFO_SERVER, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH) == -1)
		exit_err("mkfifo");
	if ((sfifo = open(FIFO_SERVER, O_RDONLY)) == -1)
		exit_err("open 1");
	
	errno = 0;
	while ((nread = read(sfifo, &req, REQ_SZ)) == REQ_SZ) {
		puts("Got request");
	}

	if (nread != 0) {
		close(sfifo);
		unlink(FIFO_CLIENT_TEMPLATE);
		exit_err("read 1");
	}

	close(sfifo);
	unlink(FIFO_CLIENT_TEMPLATE);
	puts("fifo EOF");
	exit(EXIT_SUCCESS);
}
