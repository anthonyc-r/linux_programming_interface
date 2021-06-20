#ifndef _NUM_SERVER_H
#define _NUM_SERVER_H

#include <unistd.h>

#define MQ_CLIENT_TEMPLATE "/num_server_%lld"
#define MQ_SERVER "/num_server"
#define MAX_NS 20

struct num_msg {
	pid_t pid;
	int n;
	int nums[MAX_NS];
};

#endif
