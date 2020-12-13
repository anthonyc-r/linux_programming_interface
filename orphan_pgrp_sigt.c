/**
* Demonstrate how sigttin/ou/stp are discared when sent to an orphaned process group.
*/
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/signal.h>
#include <string.h>
#include <signal.h>

void handler(int sig) {
	puts("Now disposition isn't default, it is no longer discarded...");
	_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	pid_t orphan;
	if ((orphan = fork()) == 0) {
		sleep(2);
		// Orphan pgrp
		puts("Process in orphaned pgroup continuing, no signal was received!");
		puts("orphan is now setting up a handler for sigstp...");
		if (signal(SIGTSTP, handler) == SIG_ERR) {
			puts("Failed to set handler in orphan");
			_exit(EXIT_FAILURE);
		}
		sleep(4);
		_exit(EXIT_SUCCESS);
	} else if (fork() == 0) {
		sleep(1);
		// Another child to raise the signal!
		// Oh the humanity!
		puts("Sending sigstp to process in orphaned pgroup");
		kill(orphan, SIGTSTP);
		sleep(3);
		puts("Sending sigstp to process in orphaned pgroup");
		kill(orphan, SIGTSTP);
		_exit(EXIT_SUCCESS);
	}
	
	exit(EXIT_SUCCESS);
}