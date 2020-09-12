/**
An example program showing that when a signal's disposition is set to SIG_IGN
The program never sees the signal!
*/


#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char **argv) {
	// Set disposition of SIGINT to SIG_IGN. Trying to exit the process with ctrl-c
	// will no longer work, thus showing the effect of SIG_IGN. Make terminal
	// send SIGQUIT (ctrl-\) to exit instead...
	struct sigaction sigact;
	memset(&sigact, 0, sizeof (struct sigaction));
	sigact.sa_handler = SIG_IGN;
	if (sigaction(SIGINT, &sigact, NULL) == -1) {
		puts("sigaction");
		exit(EXIT_FAILURE);
	}
	while (1)
		sleep(1);
	exit(EXIT_SUCCESS);
}