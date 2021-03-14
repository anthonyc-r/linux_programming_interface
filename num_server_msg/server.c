/**
* A version of the number client/server found in ../num_server using a
* sysv message queue.
* The server and client open a message queue using ftok(/tmp/num_server.sysv).
* The client sends it's PID with a message type of 1. The server then responds
* by sending the requested packet with a message type of the client's PID.
* 
* Naive implementation. For example, it's possible for a malicious client to 
* make many long requests without calling msgrcv, filling up the queue, 
* resulting in deadlock. 
*/

#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>

#include "server.h"

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	key_t key;
	int id, fd, num, iter;
	struct stat sb;
	struct num_server_req req;
	struct num_server_res res;
	size_t reqsz, ressz;
	
	if (stat(NUM_SERVER_PATH, &sb) == -1) {
		if ((fd = open(NUM_SERVER_PATH, O_CREAT, S_IRWXU)) == -1)
			exit_err("open path create");
		close(fd);
	}
		
	if ((key = ftok(NUM_SERVER_PATH, 0)) == (key_t)-1)
		exit_err("ftok");
	if ((id = msgget(key, IPC_CREAT | S_IRWXU)) == -1)
		exit_err("msgget");
	reqsz = sizeof (struct num_server_req) 
		- offsetof(struct num_server_req, pid);
	ressz = sizeof (struct num_server_res) 
		- offsetof(struct num_server_res, n);
	num = 0;
	
	while (1) {
		if (msgrcv(id, &req, reqsz, 1, 0) == -1) {
			if (errno == EAGAIN)
				continue;
			else
				exit_err("msgrcv");
		}
		for (iter = 0; iter < req.count; iter++) {
			res.type = (long)req.pid;
			res.n = num++;
			if (msgsnd(id, &res, ressz, 0) == -1)
				exit_err("msgsnd");
		}
		
	}
	exit(EXIT_SUCCESS);
}
