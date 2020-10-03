/**
* This program will send one million signals in series back and forth between
* itself using sigwaitinfo. Use in conjunction with timed_sigsuspend.c to
* compare the difference in performance between this method and sigsuspend.
*
* Child process sends n signals, waiting for the response from parent before
* sending the next each time. Once the child process has send n signals it
* kills the parent and exits.
* 
* Example session:
*
* $ time ./timed_sigwaitinfo 100000
* Will send 100000 signals
* Child proc 59551 started
* Child complete
* Killed
* 
* real    0m19.884s
* user    0m0.229s
* sys     0m19.615s
*/
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

static int count;
static pid_t child_pid;

static void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

static void handler_parent(int sig, siginfo_t *siginfo, void *ucontext) {
	kill(child_pid, SIGUSR1);
}

static void handler_child(int sig, siginfo_t *siginfo, void *ucontext) {

}

static void child_proc(int nsend) {
	struct sigaction sigact;
	sigset_t maskset;
	
	printf("Child proc %ld started\n", (long)getpid());
	
	sigfillset(&maskset);
	sigfillset(&sigact.sa_mask);
	sigact.sa_sigaction = handler_child;
	sigact.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sigact, NULL);

	while (count < nsend) {
		sigprocmask(SIG_BLOCK, &maskset, NULL);
		// Super important work we don't want interupted...
		kill(getppid(), SIGUSR1);
		count++;
		sigwaitinfo(&maskset, NULL);
	}
	puts("Child complete");
	kill(getppid(), SIGKILL);
	exit(EXIT_SUCCESS);
}

static void parent_proc() {
	struct sigaction sigact;
		
	sigfillset(&sigact.sa_mask);
	sigact.sa_sigaction = handler_parent;
	sigact.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sigact, NULL);
	
	while (1) {
		sleep(1);
	}
	
	puts("Parent complete");
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
	if (argc < 2)
		error("provide number of signals to send");
	int nsend = (int)strtol(argv[1], NULL, 10);
	printf("Will send %d signals\n", nsend);
	switch ((child_pid = fork())) {
		case 0:
			// child
			child_proc(nsend);
			break;
		case -1:
			// error
			error("fork");
			break;
		default:
			// parent
			parent_proc();
			break;
	}
	exit(EXIT_FAILURE);
}
