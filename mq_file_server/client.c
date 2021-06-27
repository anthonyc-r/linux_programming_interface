#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>
#include <mqueue.h>

#include "server.h"

static char qpath[PATH_MAX];
static mqd_t qid;

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void remove_id() {
	puts("Removing ID");
	mq_close(qid);
	mq_unlink(qpath);
}

int main(int argc, char **argv) {
	mqd_t qserver;
	struct mq_attr qattr;
	int rcv, fd;
	char *outfile;
	union file_msg msg;
	struct stat sb;

	if (argc < 2) {
		puts("usage: ./client <filepath>");
		exit(EXIT_FAILURE);
	}


	if ((outfile = basename(argv[1])) == NULL)
		exit_err("basename");
	if (stat(outfile, &sb) != -1)
		exit_err("output file exists - not overwriting");
	if ((fd = open(outfile, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR )) == -1)
		exit_err("open O_CREAT");

	qattr.mq_maxmsg = 10;
	qattr.mq_msgsize = sizeof (union file_msg);
	snprintf(qpath, PATH_MAX, "/file_server_client_%ld", (long)getpid());
	if ((qid = mq_open(qpath, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXO, &qattr)) == (mqd_t)-1)
		exit_err("mq_open client");
	atexit(remove_id);

	if ((qserver = mq_open(MQ_PATH, O_WRONLY)) == (mqd_t)-1)
		exit_err("mq_open server (Is the server running?)");

	strncpy(msg.req.qpath, qpath, PATH_MAX);
	strncpy(msg.req.path, argv[1], PATH_MAX);
	if (mq_send(qserver, (char*)&msg, sizeof (union file_msg), 1) == -1)
		exit_err("mq_send");
	while (mq_receive(qid, (char*)&msg, sizeof (union file_msg), NULL) != -1) {
		switch (msg.part.type) {
		case MSG_PART:
			if (write(fd, msg.part.data, msg.part.n) != msg.part.n)
				exit_err("write");
			printf("MSG_PART|");
			break;
		case MSG_DONE:
			puts("MSG_DONE");
			goto done;
		case MSG_OPEN_ERROR:
			exit_err("Server sent MSG_OPEN_ERROR");
		case MSG_READ_ERROR:
			exit_err("Server sent MSG_READ_ERROR");
		default:
			break;
		}
	}
done:
	close(fd);
	if (rcv == -1)
		exit_err("msgrcv");

	exit(EXIT_SUCCESS);
}
