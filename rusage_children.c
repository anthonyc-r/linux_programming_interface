/*
* Demonstrate how getrusage RUSAGE_CHILDREN only shows the usage for reaped child processes.
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

static void ptval(struct timeval *tval) {
	printf("%.2f", (double)tval->tv_sec + (double)tval->tv_usec / 1000000);
}

static void pinfo() {
	struct rusage rusage;
	struct timeval *time;

	if (getrusage(RUSAGE_CHILDREN, &rusage) == -1)
		exit_error();
	
	printf("RUSAGE_CHILDREN - user: ");
	ptval(&rusage.ru_utime);
	printf(", sys: ");
	ptval(&rusage.ru_stime);
	puts("");
}

static void chexit(int sig) {
	puts("Child exiting!");
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
	pid_t child;
	int i, waitr, fd;
	
	puts("Spawning child");
	if ((child = fork()) == 0) {
		signal(SIGALRM, chexit);
		alarm(1);
		// Use some cputime...
		fd = open("/dev/null", O_WRONLY);
		while (1)
			write(fd, "trash", 5);
	} else if (child == -1) {
		exit_error();
	}
	for (i = 0; i < 10; i++) {
		sleep(1);
		pinfo();
		if (i == 5) {
			puts("Reaping child");
			wait(&waitr);
		}
	}	

	exit(EXIT_SUCCESS);
}
