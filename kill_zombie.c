/*
* This program demonstrates that zombies can't be killed, even by the sure
* kill signal.
* Although this program uses signals to sync parent and child, as instructed
* by the exercise, it could also be done with waitid() + WNOWAIT flag.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <libgen.h>

static void handler(int sig) {

}

static void err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	sigset_t block;
	char cmd[100];
	pid_t child;
	// Don't really care about the exact semantics of this
	if (signal(SIGCHLD, handler) == SIG_ERR)
		err("signal");
	
	snprintf(cmd, 100, "pgrep %s", basename(argv[0]));

	sigemptyset(&block);
	sigaddset(&block, SIGCHLD);
	sigprocmask(SIG_BLOCK, &block, NULL);		
	
	switch ((child = fork())) {
		case -1:
			err("fork");
		case 0:
			puts("Child exiting");
			_exit(EXIT_SUCCESS);
		default:
			puts("Waiting for child to exit");
			sigemptyset(&block);
			sigsuspend(&block);
			if (system(cmd) == -1)
				err("system");
			puts("Send SIGKILL");
			kill(child, SIGKILL);
			if (system(cmd) == -1)
				err("system");
	}
	exit(EXIT_SUCCESS);
}
