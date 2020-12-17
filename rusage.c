/*
* A command line util to measure the resource usage of the specified program
* ./rusage command arg...
*
* Note the use of sigprocmask so only the child receives terminal generated signals.
* Tested on openbsd
*/
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


static void exit_error() {
	printf("error: %s\n", strerror(errno));
	exit(errno);
}

static void exit_usage() {
	puts("./rusage command arg...");
	exit(EXIT_FAILURE);
}

static void ptval(struct timeval *tval) {
	printf("%.2f", (double)tval->tv_sec + (double)tval->tv_usec / 1000000);
}

static void plong(char *msg, long *val) {
	printf("%s: %ld\n", msg, *val);
}

static void pinfo() {
	struct rusage rusage;
	struct timeval *time;

	if (getrusage(RUSAGE_CHILDREN, &rusage) == -1)
		exit_error();
	
	printf("user time used: ");
	ptval(&rusage.ru_utime);
	puts("");
	printf("system time used: ");
	ptval(&rusage.ru_stime);
	puts("");
	plong("max resident set size", &rusage.ru_maxrss);
	plong("integral shared text memory size", &rusage.ru_ixrss);
	plong("integral unshared data size", &rusage.ru_idrss);
	plong("integral unshared stack size", &rusage.ru_isrss);
	plong("page reclaims", &rusage.ru_minflt);
	plong("page faults", &rusage.ru_majflt);
	plong("swaps", &rusage.ru_nswap);
	plong("block input operations", &rusage.ru_inblock);
	plong("block output operations", &rusage.ru_oublock);
	plong("messages sent", &rusage.ru_msgsnd);
	plong("messages received", &rusage.ru_msgrcv);
	plong("signals received", &rusage.ru_nsignals);
	plong("voluntary context switches", &rusage.ru_nvcsw);
	plong("involuntary context switches", &rusage.ru_nivcsw);	
}

int main(int argc, char **argv) {
	pid_t child;
	int i, waitr, fd;
	sigset_t block, prev;

	if (argc < 2)
		exit_usage();
	
	if (sigfillset(&block) == -1)
		exit_error();
	if (sigprocmask(SIG_BLOCK, &block,  &prev) == -1)
		exit_error();
	if ((child = fork()) == 0) {
		if (sigprocmask(SIG_SETMASK, &prev, NULL) == -1)
			exit_error();
		execvp(argv[1], argv + 1);
		exit_error();
	} else if (child == -1) {
		exit_error();
	}
	if (wait(&waitr) == -1)
		exit_error();
	pinfo();

	exit(EXIT_SUCCESS);
}
