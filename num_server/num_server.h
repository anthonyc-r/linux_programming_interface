#ifndef _NUM_SERVER_H
#define _NUM_SERVER_H

#include <unistd.h>

#define FIFO_CLIENT_TEMPLATE "/tmp/num_server_%lld.fifo"
#define FIFO_SERVER "/tmp/num_server.fifo"

struct num_open_req {
	pid_t pid;
	int nreq;
};

struct num_response {
	int nres;
	int nums[];
};

#endif