#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>

#include "server.h"

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	key_t tok;
	int id, iter;
	struct num_server_req req;
	struct num_server_res res;
	size_t reqsz, ressz;
	
	if ((tok = ftok(NUM_SERVER_PATH, 0)) == (key_t)-1)
		exit_err("ftok");
	if ((id = msgget(tok, 0)) == -1)
		exit_err("msgget");
	reqsz = sizeof (struct num_server_req) 
		- offsetof(struct num_server_req, pid);
	ressz = sizeof (struct num_server_res) 
		- offsetof(struct num_server_res, n);
	
	req.pid = getpid();
	req.type = 1;
	req.count = 5;
	if (msgsnd(id, &req, reqsz, 0) == -1)
		exit_err("msgsnd");
	
	for (iter = 0; iter < req.count; iter++) {
		if (msgrcv(id, &res, ressz, getpid(), 0) == -1)
			exit_err("msgrcv");
		printf("Got number: %d\n", res.n);
	}
	exit(EXIT_SUCCESS);
}
