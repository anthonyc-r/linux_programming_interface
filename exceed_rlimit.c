/*
* Check what happens when we set the soft limit lower than the existing value for various
* resource limits.
* Specifically: 
* RLIMIT_CPU - maximum cpu usage. Usual behavior is to receieve SIGXCPU.
* RLIMIT_DATA - maximum data segmnt. Usually will cause sbrk/brk (thus malloc) to error
* RLIMIT_NPROC - maximum processes.
* Tested on OpenBSD.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

extern char end;

static void exit_error() {
	printf("error: %s\n", strerror(errno));
	exit(errno);
}

static void exit_errorn(int nerrno) {
	printf("error: %s\n", strerror(nerrno));
	exit(nerrno);
}

int main(int argc, char **argv) {
	sigset_t block;
	struct rlimit rlim;
	int sig, res;
	pid_t pid;
	void *heap1, *heap2;
	void *cbrk;
	
	sigemptyset(&block);
	sigaddset(&block, SIGXCPU);
	sigprocmask(SIG_BLOCK, &block, NULL);

	if (getrlimit(RLIMIT_CPU, &rlim) == -1)
		exit_error();
	puts("Setting RLIMIT_CPU soft to 0!");
	rlim.rlim_cur = 0;
	if (setrlimit(RLIMIT_CPU, &rlim) == -1)
		exit_error();
	if ((res = sigwait(&block, &sig)) != 0)
		exit_errorn(res);
	if (sig == SIGXCPU) {
		puts("Got SIGXCPU! Resetting to sensible value...");
		rlim.rlim_cur = rlim.rlim_max;
		if (setrlimit(RLIMIT_CPU, &rlim) == -1)
			exit_error();
	}
	puts("Setting RLIMIT_NPROC to 0!");
	if (getrlimit(RLIMIT_NPROC, &rlim) == -1)
		exit_error();
	rlim.rlim_cur = 0;
	if (setrlimit(RLIMIT_NPROC, &rlim) == -1)
		exit_error();
	puts("Forking!");
	if ((pid = fork()) == 0) {
		exit(EXIT_SUCCESS);
	}

	if (pid != -1)
		puts("Result of fork success?!"); 
	else 
		printf("Result of fork was: %s\n", strerror(errno));
	
	puts("Allocating a 1KiB chunk of memory");
	heap1 = malloc(1024);
	if (heap1 == NULL) {
		exit_error();
	}
	puts("Setting RLIMIT_DATA to 0!");
	if (getrlimit(RLIMIT_DATA, &rlim) == -1)
		exit_error();
	rlim.rlim_cur = 0;
	if (setrlimit(RLIMIT_DATA, &rlim) == -1)
		exit_error();
	puts("RLIMT_DATA is now 0, allocating one byte on the heap...");
	heap2 = malloc(1);
	if (heap2 == NULL) {
		printf("malloc failed with error: %s\n", strerror(errno));
	}
	puts("Freeing 1KiB block");
	free(heap1);
	puts("Attempt to allocate one byte...");
	heap2 = malloc(1);
	if (heap2 == NULL) {
		printf("malloc failed with error: %s\n", strerror(errno));
	}
	

	exit(EXIT_SUCCESS);
}