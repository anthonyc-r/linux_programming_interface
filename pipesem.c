/**
* Binary semaphore using named pipes.
* reserve, release cond reserve.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>


int release(int fd) {
	char buf[100];
	
	// Drain the pipe first.
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	while (read(fd, &buf, 100) != -1)
		;
	if (fcntl(fd, F_SETFL, 0) == -1)
		return -1;
	return write(fd, &buf, 1);
}

int reserve(int fd) {
	char buf;
	
	if (fcntl(fd, F_SETFL, 0) == -1)
		return -1;
		
	return read(fd, &buf, 1);
}

int reserveCond(int fd) {
	char buf;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return read(fd, &buf, 1);
}

// Test it out

static void handler(int sig) {

}

int main(int argc, char **argv) {
	int fd;
	struct sigaction sigact;
	
	// Make a fifo to use.
	mkfifo("/tmp/test_sem", S_IRWXU);
	if ((fd = open("/tmp/test_sem", O_RDWR)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	
	
	// Setup a timeout 
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = handler;
	sigaction(SIGALRM, &sigact, NULL);
	alarm(1);
	
	// Reserve it. Should block.
	puts("reserve");
	if (reserve(fd) == -1) {
		puts("timed out");
	}
	alarm(0);
	
	// Release.
	puts("Release");
	release(fd);
	puts("Release");
	release(fd);
	puts("Release");
	release(fd);
	
	switch (fork()) {
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
	case 0:
		puts("[Child] Reserving ..");
		reserve(fd);
		puts("[Child] Doing some work..");
		sleep(2);
		puts("[Child] Releasing...");
		release(fd);
		exit(EXIT_SUCCESS);
	default:
		puts("[Parent] Doing some work...");
		sleep(1);
		puts("[Parent] Reserving...");
		if (reserveCond(fd) == -1)
			puts("[Parent] Sem already reserved.. reserve anyway.");
		reserve(fd);
		puts("[Parent] Has resource!");
		release(fd);
	}
	

	//unlink("/tmp/test_sem");
	puts("");
	exit(EXIT_SUCCESS);
}
