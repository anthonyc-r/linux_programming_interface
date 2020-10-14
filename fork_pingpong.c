/*
* This program demonstraits a child process waiting on a parent process to 
* perform some task, and then the parent process waits for the child process
* to perform some task, and back and forth.
*/
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

#define REPEAT 5

static pid_t pid;
static int count;

static void handler(int sig) {
}

static void run_parent() {
	int i, status;
	sigset_t empty;
	sigemptyset(&empty);

	kill(pid, SIGUSR1);
	for (i = 0; i < REPEAT; i++) {
		// Wait for the go ahead
		sigsuspend(&empty);
		// Do some work
		sleep(1);
		puts("(parent) Okay, go ahead!");
		// Signal go ahead!
		kill(pid, SIGUSR1);
	}
	kill(pid, SIGINT);
	wait(&status);
}

static void run_child() {
	sigset_t empty;
	sigemptyset(&empty);

	while (1) {
		// Wait for the go ahead
		sigsuspend(&empty);
		// Do some work..
		sleep(1);
		puts("(child) All done!");
		kill(pid, SIGUSR1);
	}
}

int main(int args, char **argv) {
	sigset_t block;
	struct sigaction sigact;

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = handler;
	if (sigaction(SIGUSR1, &sigact, NULL) == -1) {
		puts("sigaction");
		exit(EXIT_FAILURE);
	}

	sigemptyset(&block);
	sigaddset(&block, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &block, NULL) == -1) {
		puts("sigprocmask");
		exit(EXIT_FAILURE);
	}
	switch ((pid = fork())) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			pid = getppid();
			run_child();
			break;
		default:
			run_parent();
	}
	exit(EXIT_SUCCESS);
}
