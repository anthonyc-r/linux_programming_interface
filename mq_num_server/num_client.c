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
#include <mqueue.h>
#include "num_server.h"

#define MQ_PERMS S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	char mpath[PATH_MAX];
	int smq, cmq, i;
	struct num_msg msg;
	struct mq_attr mqattr;

	msg.pid = getpid();
	msg.n = 5;
	snprintf(mpath, PATH_MAX, MQ_CLIENT_TEMPLATE, (long long)getpid());
	// Make our message queue
	mqattr.mq_maxmsg = 10;
	mqattr.mq_flags = 0;
	mqattr.mq_msgsize = sizeof (struct num_msg);
	if ((cmq = mq_open(mpath, O_RDONLY | O_CREAT, MQ_PERMS, &mqattr)) == -1)
		exit_err("mq_open client");

	smq = mq_open(MQ_SERVER, O_WRONLY);
	if (mq_send(smq, (char *)&msg, sizeof (struct num_msg), 1) == -1)
		exit_err("mq_send - num_server probably isn't running.");

	if (mq_receive(cmq, (char*)&msg, sizeof (struct num_msg), NULL) == -1)
		exit_err("mq_receive");
	printf("Got response with sequence length: %d\n", msg.n);
	for (i = 0; i < msg.n; i++) {
		printf("%d, ", msg.nums[i]);
	}
	puts("");
	mq_close(cmq);
	exit(EXIT_SUCCESS);
}
