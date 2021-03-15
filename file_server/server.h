#ifndef _SERVER_H
#define _SERVER_H

#include <limits.h>

#define DATASZ 1024
#define MSG_PART 1
#define MSG_NOT_FOUND 2
#define MSG_READ_ERROR 3

struct file_request {
	long type;
	int qid;
	char path[PATH_MAX];
}

struct file_part {
	long type;
	char data[DATASZ];
}

#endif
