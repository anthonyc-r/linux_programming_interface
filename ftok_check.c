/**
Verify the implementation of ftok() function.
On linux the result is expected to be composed of 
proj, inode number, device id
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

#define KEY_FILE "/tmp/ftok_check.sysv"
#define PROJ 1

int main(int argc, char **argv) {
	struct stat sb;
	int fd;
	key_t tok;
	
	// Create the key file if it doesn't exist.
	if (stat(KEY_FILE, &sb) == -1) {
		fd = open(KEY_FILE, O_CREAT | O_RDWR, S_IRWXU);
		close(fd);
	} 
	tok = ftok(KEY_FILE, PROJ);
	printf("FTOK:\t%08llx\n", (long long)tok);
	if (stat(KEY_FILE, &sb) == -1)
		exit(EXIT_FAILURE);
	
	printf("INO:\t%08llx\n", (long long)sb.st_ino);
	printf("DEV:\t%08llx\n", (long long)sb.st_dev);
	printf("PRJ:\t%08x\n", PROJ);
	exit(EXIT_SUCCESS);
}
