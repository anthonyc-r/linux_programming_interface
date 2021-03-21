#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>

#include "server.h"

static int id;

static void sig(int s) {

}

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void setup_sig() {

}

static void remove_id() {
	puts("Removing ID");
	unlink(ID_FILE);
	msgctl(id, IPC_RMID, 0);
}

static void create_id() {
	char *tmp;
	int fd;
	
	if ((id = msgget(IPC_PRIVATE, S_IRWXU | S_IROTH)) == -1)
		exit_err("msgget");
	if ((tmp = tmpnam(NULL)) == NULL)
		exit_err("tmpnam");
	if ((fd = open(tmp, O_WRONLY | O_CREAT, S_IRWXU)) == -1)
		exit_err("open");
	if (write(fd, &id, sizeof(int)) != sizeof(int))
		exit_err("write");
	close(fd);
	if (rename(tmp, ID_FILE) == -1)
		exit_err("rename");
}

static int get_request(struct file_request *req) {
	size_t sz;
	
	sz = sizeof(struct file_request) - offsetof(struct file_request, qid);
	if (msgrcv(id, req, sz, 0, 0) == -1)
		return -1;
	return 0;
}

static void handle_request(struct file_request *req) {
	int fd;
	int res;
	struct file_part part;
	
	if (open(req->path, O_RDONLY) == -1) {
		res = MSG_OPEN_ERROR;
		msgsnd(req->qid, &res, 0, 0);
		return;
	}
	
	part.type = MSG_PART;
	errno = 0;
	while (read(fd, part.data, DATASZ) > 0) {
		msgsnd(req->qid, &part, DATASZ, 0);
	}
	if (errno != 0) {
		res = MSG_READ_ERROR;
		msgsnd(req->qid, &res, 0, 0);
		return;
	} else {
		res = MSG_DONE;
		msgsnd(req->qid, &res, 0, 0);
	}
}

int main(int argc, char **argv) {
	struct file_request req;
	
	setup_sig();
	atexit(remove_id);
	create_id();
	while (get_request(&req) != -1) {
		handle_request(&req);
	}
	exit_err("get_request");
}
