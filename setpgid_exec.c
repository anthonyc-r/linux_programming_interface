/**
* Verify that it's possible to change the process group id of a forked() child before exec,
* but not after it execs.
*/
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

static void handler(int sig) {
	// Not asycn signal safe...
	printf("Child's PGID: %lu \n", (unsigned long)getpgrp());
	raise(SIGSTOP);
}

static void err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	struct sigaction sigact;
	pid_t child;

	sigact.sa_handler = handler;
	sigact.sa_flags = 0;
	if (sigemptyset(&sigact.sa_mask) == -1)
		err("sigemptyset");
	if(sigaction(SIGUSR1, &sigact, NULL) == -1)
		err("sigaction");

	// If called with an argument, it's the child that has been exec'd.
	// Simply wait for a signal.
	while (argc > 1) {
		pause();
	}
	
	// Alarm applies to forked, and even exec'd children.
	alarm(60);
	printf("PGID: %lu\n", (unsigned long)getpgrp());
	puts("Attempting to change a non-exec'd child's process group");
	switch ((child = fork())) {
	case -1:
		err("fork");
	case 0:
		// Child. On this run, don't exec. Just wait for a signal.
		while (1) {
			pause();
		}
		break;
	default:
		// Parent. Attempt ot change the childs process group, and ask it to print.
		sleep(1);
		if (setpgid(child, child) == -1)
			puts("setpgid failed!");
		kill(child, SIGUSR1);
		break;
	}

	puts("Attempting to change a child's pgrp after exec!");
	switch ((child = fork())) {
	case -1:
		err("fork");
	case 0:
		// Child. Exec this program, but with an arg; see above.
		execl(argv[0], argv[0], "x", (const char*)NULL);
		err("execl");
		break;
	default:
		// Parent. Attempt to change the childs process group, and ask it to print.
		sleep(1);
		if (setpgid(child, child) == -1)
			puts("setpgid failed!");
		kill(child, SIGUSR1);
		break;
	}
	sleep(1);
	exit(EXIT_SUCCESS);
}