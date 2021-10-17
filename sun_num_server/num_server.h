#ifndef _NUM_SERVER_H
#define _NUM_SERVER_H

#include <unistd.h>

#define SERVER_NAME "num_server"
#define MAX_N 100

struct num_greeting {
	int nreq;
};

struct num_response {
	int nums[MAX_N + 1];
};

#define GREETSZ sizeof (struct num_greeting)
#define RESSZ sizeof (struct num_response)

#endif
