/*
An implementation of 'alarm()' using setitimer
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>

unsigned int alarm(unsigned int seconds) {
	struct itimerval timerval;
	
	timerval.it_interval.tv_sec = 0;
	timerval.it_interval.tv_usec = 0;
	timerval.it_value.tv_sec = seconds;
	timerval.it_value.tv_usec = 0;
	return setitimer(ITIMER_REAL, &timerval, NULL);
}

void sighandler(int sig) {
	//not async signal safe...
	printf("bzzzzzz\n");
}

int main(int argc, char **argv) {
	sigset_t block;
	struct sigaction sigact;
	time_t seconds;

	if (argc < 2) {
		printf("Usage 'alarm <nseconds>'\n");	
		exit(EXIT_FAILURE);
	}
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sighandler;
	if (sigaction(SIGALRM, &sigact, NULL) == -1) {
		printf("sigaction\n");
		exit(EXIT_FAILURE);
	}

	seconds = strtol(argv[1], NULL, 10);
	sigemptyset(&block);
	sigaddset(&block, SIGALRM);
	sigprocmask(SIG_BLOCK, &block, NULL);
	if (alarm(seconds) == -1) {
		printf("alarm\n");
		exit(EXIT_FAILURE);
	}
	sigemptyset(&block);
	sigsuspend(&block);
	exit(EXIT_SUCCESS);
}
