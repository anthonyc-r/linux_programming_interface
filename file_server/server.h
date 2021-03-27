#ifndef _SERVER_H
#define _SERVER_H

#include <limits.h>
#include <stddef.h>

#define ID_FILE "/tmp/file_server_id"
#define DATASZ 1024

#define MSG_PART 1
#define MSG_DONE 2
#define MSG_OPEN_ERROR 3
#define MSG_READ_ERROR 4
#define MSG_REQ 100

struct file_request {
	long type;
	int qid;
	char path[PATH_MAX];
};

struct file_part {
	long type;
	ssize_t n;
	char data[DATASZ];
};

#define PARTSZ (sizeof(struct file_part) - offsetof(struct file_part, data))
#define REQSZ (sizeof(struct file_request) - offsetof(struct file_request, qid))


#endif
