/**
* Demonstrate how attempts by an member of an orphaned process group to read from the 
* controlling terminal fail with EIO.
*/
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/signal.h>
#include <string.h>

int main(int argc, char **argv) {
	char a;
	// Create orphan pgroup. (No process in the pgroup has a parent in the same session
	// But in a different pgroup.)
	if (fork() == 0) {
		sleep(1);
		// No longer in the foreground pgroup as the parent exited.
		if (read(STDIN_FILENO, &a, 1) == -1 && errno == EIO) {
			printf("read() failed with error: %s\n", strerror(errno));
		}
		printf("process: %d, pgroup: %d\n", getpid(), getpgrp());
		// Attempt to make this the forground pgroup and retry...
		if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
			printf("tcsetpgrp failed with error: %s\n", strerror(errno));
			// Doesn't let us?
			
			exit(EXIT_FAILURE);
		}
	}
	exit(EXIT_SUCCESS);
}