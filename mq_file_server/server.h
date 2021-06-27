#ifndef _SERVER_H
#define _SERVER_H

#include <limits.h>
#include <stddef.h>

#define MQ_PATH "/file_server_id"
#define DATASZ 1024

#define MSG_PART 1
#define MSG_DONE 2
#define MSG_OPEN_ERROR 3
#define MSG_READ_ERROR 4
#define MSG_REQ 100

struct file_request {
	char qpath[PATH_MAX];
	char path[PATH_MAX];
};
struct file_part {
	int type;
	ssize_t n;
	char data[DATASZ];
};
union file_msg {
	struct file_request req;
	struct file_part part;
};

#endif
