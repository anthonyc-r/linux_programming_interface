/**
* Investigate kernel-side implementation of Xget sysv ipc calls. 
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/stat.h>

#define KEY_FILE_1 "/tmp/ftok_check.sysv"
#define KEY_FILE_2 "/tmp/ftok_check_2.sysv"
#define PROJ 1

static key_t get_tok(char *f) {
	struct stat sb;
	int fd;
	
	if (stat(f, &sb) == -1) {
		fd = open(f, O_CREAT | O_RDWR, S_IRWXU);
		close(fd);
	}
	return ftok(f, PROJ);
}


int main(int argc, char **argv) {
	int id, id2, i;
	key_t tok1, tok2;

	
	tok1 = get_tok(KEY_FILE_1);
	tok2 = get_tok(KEY_FILE_2);
	
	printf("TOK:\t%lx\n", tok1);
	puts("Creating objects and closing them with the same KEY");
	for (i = 0; i < 10; i++) {
		if ((id = msgget(tok1, S_IRWXU | O_RDONLY | O_CREAT)) == -1)
			exit(EXIT_FAILURE);
		printf("ID:\t%08x\n", id);
		msgctl(id, IPC_RMID, NULL);
	}
	
	puts("Creating two objects with different keys");
	if ((id = msgget(tok1, S_IRWXU | O_RDONLY | O_CREAT)) == -1)
			exit(EXIT_FAILURE);
	if ((id2 = msgget(tok2, S_IRWXU | O_RDONLY | O_CREAT)) == -1)
			exit(EXIT_FAILURE);
			
	printf("ID:\t%08x\n", id);
	printf("ID:\t%08x\n", id2);
	msgctl(id, IPC_RMID, NULL);
	msgctl(id2, IPC_RMID, NULL);
	
	exit(EXIT_SUCCESS);
}
