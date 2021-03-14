#ifndef _SERVER_H
#define _SERVER_H

#define NUM_SERVER_PATH "/tmp/num_server.sysv"

struct num_server_req {
	long type;
	pid_t pid;
	int count;
};

struct num_server_res {
	long type;
	int n;
};

#endif
