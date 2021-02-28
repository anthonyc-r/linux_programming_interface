/**
* An implementation of ftok.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define KEY_FILE "/tmp/ftok_check.sysv"
#define PROJ 1

typedef int mkey_t ;

mkey_t mftok(const char *path, int id) {
	struct stat sb;
	mkey_t ret;

	if (stat(path, &sb) == -1)
		return -1;
	ret = 0;
	ret |= sb.st_ino & 0xFFFF;
	ret |= (sb.st_dev & 0xFF) << 16;
	ret |= (id & 0xFF) << 24;
	return ret;
}

int main(int argc, char **argv) {
	struct stat sb;
	int fd;
	mkey_t mres;
	key_t res;

	if (stat(KEY_FILE, &sb) == -1) {
		fd = open(KEY_FILE, O_CREAT | O_RDWR, S_IRWXU);
		close(fd);
	} 
	
	res = ftok(KEY_FILE, PROJ);
	mres = mftok(KEY_FILE, PROJ);
	printf("Sys:\t%08lx\n", res);
	printf("Mine:\t%08x\n", mres); 
	exit(EXIT_SUCCESS);
}
