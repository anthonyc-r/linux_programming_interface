/*
* This program attempts to demonstrate interactions between system() and 
* handlers of sigchld.
*/
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>

void handler(int sig) {
	int waitcode;
	// unsafe..
	printf("handler called!\n");
	if (wait(&waitcode) == -1 && errno == ECHILD) {
		printf("Nothing to wait for...\n");
	} else {
		printf("Successfully reaped child\n");
	}
}

int main(int argc, char **argv) {
	sigset_t block;
	int waitstat;

	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, NULL);
	signal(SIGCHLD, handler);

	switch (fork()) {
		case -1:
			exit(EXIT_FAILURE);
		case 0:
			_exit(EXIT_SUCCESS);
		default:
			break;
	}

	system("echo sys");
	
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	wait(&waitstat);
	
	exit(EXIT_SUCCESS);
}