/**
 * Does flock() detect deadlock situations and return an error?
**/


#include <unistd.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <stdio.h>

#define FILE1 "/tmp/deadflock1.tmp"
#define FILE2 "/tmp/deadflock2.tmp"

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int fd1, fd2;


	unlink(FILE1);
	unlink(FILE2);

	switch(fork()) {
	case -1:
		exit_err("fork");
	case 0:
		// Have to open them in the child process, as lock object is
		// associated with the open file description, rather than fd...
		if ((fd1 = open(FILE1, O_CREAT | O_RDWR, S_IRWXU)) == -1)
			exit_err("open1");
		if ((fd2 = open(FILE2, O_CREAT | O_RDWR, S_IRWXU)) == -1)
			exit_err("open2");

		puts("[Child] Lock fd1");
		if (flock(fd1, LOCK_EX) == -1)
			exit_err("fd1");
		sleep(1);
		puts("[Child] @T = 1s: P1 <-> fd1 && P2 <-> fd2");
		puts("[Child] We try to flock fd2, and block.");
		if (flock(fd2, LOCK_EX) == -1)
			exit_err("fd2");
		puts("[Child] DONE");
		exit(EXIT_SUCCESS);
	default:
		if ((fd1 = open(FILE1, O_CREAT | O_RDWR, S_IRWXU)) == -1)
			exit_err("open1");
		if ((fd2 = open(FILE2, O_CREAT | O_RDWR, S_IRWXU)) == -1)
			exit_err("open2");


		puts("[Parent] Lock fd2");
		if (flock(fd2, LOCK_EX) == -1)
			exit_err("fd2");
		sleep(2);
		puts("[Parent] Lock fd1. Deadlock.");
		if (flock(fd1, LOCK_EX) == -1)
			exit_err("fd1");
	}

	puts("[Parent] DONE");
	unlink(FILE1);
	unlink(FILE2);
	exit(EXIT_SUCCESS);
}
