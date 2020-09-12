/**
Demonstrate the fact that signals are not queued up. So if multiple signals of the same type
are sent while the process mask is blocking them, upon unmasking them, you only receieve 1.

Block all signals, then sleep. While counting the number of signals receieved.
Use sigaction() instead of signal()
Note that this program uses stdio functions within the signal handlers,
which is generally unsafe.
*/

// NSIG
#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define SEND_COUNT 1000000

static int counts[NSIG];
static sigset_t blockset;

static void error(char *string) {
	puts(string);
	exit(EXIT_FAILURE);
}

static void print_counts() {
	int i, count;
	
	printf("\nCounts:\n");
	for (i = 0; i < NSIG; i++) {
		count = counts[i];
		if (count > 0) {
			// Unsafe
			printf("\t%s: %d\n", strsignal(i), count);
		}
	}
}

static void shandler(int signal) {
	// Unsafe
	counts[signal]++;
	if (signal == SIGINT) {
		printf("\nClearing process mask\n");
		if (sigprocmask(SIG_UNBLOCK, &blockset, NULL) == -1)
			error("sigprocmask2");
	} else if (signal == SIGQUIT) {
		print_counts();
		exit(EXIT_SUCCESS);
	} 
}

static void run_receiver() {
	int i;
	struct sigaction sigact;
	
	memset(&sigact, 0, sizeof (struct sigaction));
	sigact.sa_handler = shandler;
	if (sigfillset(&blockset) == -1)
		error("sigfillset");
	if (sigdelset(&blockset, SIGINT) == -1)
		error("sigdelset 1");
	if (sigdelset(&blockset, SIGQUIT) == -1)
		error("sigdelset 2");
	if (sigprocmask(SIG_BLOCK, &blockset, NULL) == -1)
		error("sigprocmask1");
	for (i = 0; i < NSIG; i++) {
		sigaction(i, &sigact, NULL);
	}
	while (1)
		sleep(1);
	exit(EXIT_SUCCESS);
}

static void run_sender(pid_t pid) {
	int i;
	
	sleep(1);
	for (i = 0; i < SEND_COUNT; i++)
		kill(pid, SIGUSR1);
	kill(pid, SIGINT);
	sleep(1);
	kill(pid, SIGQUIT);
	sleep(1);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	pid_t pid;
	
	switch ((pid = fork())) {
		case 0:
			run_receiver();
		case -1:
			error("fork");
		default:
			run_sender(pid);
	}
}	
