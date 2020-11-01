/*
* This program is for measuring the difference in speed between fork() and vfork() calls.
* On openBSD, the times appear to be the same... vfork implemented with plain fork?
*/
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NFORK 50000


int main(int argc, char **argv) {
	int i, status;
	time_t start;
	void *stuff;
	
	stuff = malloc(10000000);
	start = time(NULL);
	for (i = 0; i < NFORK; i++) {
		switch (fork()) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			_exit(0);
		default:
			wait(&status);
		}
	}
	printf("Took %lld seconds to fork %d processes\n", time(NULL) - start, NFORK);
	
	start = time(NULL);
	for (i = 0; i < NFORK; i++) {
		switch (vfork()) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			_exit(0);
		default:
			wait(&status);
		}
	}
	printf("Took %lld seconds to vfork %d processes\n", time(NULL) - start, NFORK);
	
	exit(EXIT_SUCCESS);
}