#ifndef _SERVER_H
#define _SERVER_H

#include <limits.h>

#define ID_FILE "/tmp/file_server_id"
#define DATASZ 1024
#define MSG_PART 1
#define MSG_DONE 2
#define MSG_OPEN_ERROR 3
#define MSG_READ_ERROR 4

struct file_request {
	long type;
	int qid;
	char path[PATH_MAX];
};

struct file_part {
	long type;
	char data[DATASZ];
};

#endif
