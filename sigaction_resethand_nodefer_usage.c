/**
Demonstrate the usage of SA_RESETHAND and SA_NODEFER flags used with
sigaction.
SA_RESETHAND (also usable with SA_ONESHOT which gives away what it does!) resets
	the signal disposition to default after the first time it is caught.
SA_NODEFER turns off the default behaviour or adding the caught signal to the
	process mask while it's handled. This means you can get stuck in a loop if
	the signal is triggered in it's own handler.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static void sigUrgHandler1(int signal) {
	// Unsafe!!
	puts("SIGURG Handler!");
}

static void sigUrgHandler2(int signal) {
	puts("SIGURG Handler!");
	puts("Raising SIGURG from within SIGURG Handler! (ctrl-c to interrupt)");
	raise(SIGURG);
}

int main(int argc, char **argv) {
	struct sigaction sigact;
	memset(&sigact, 0, sizeof (struct sigaction));
	
	puts("Set disposition of SIGURG to sigUrgHandler1 with SA_RESETHAND");
	sigact.sa_flags = SA_RESETHAND;
	sigact.sa_handler = sigUrgHandler1;
	sigaction(SIGURG, &sigact, NULL);
	puts("Raising SIGURG");
	raise(SIGURG);
	puts("Raising SIGURG (handler was reset by SA_RESETHAND)");
	raise(SIGURG);
	
	puts("Set disposition of SIGURG to sigUrgHandler2 WITHOUT SA_NODEFER");
	sigact.sa_handler = sigUrgHandler2;
	sigaction(SIGURG, &sigact, NULL);
	puts("Raising SIGURG");
	raise(SIGURG);
	
	puts("Set disposition of SIGURG to sigUrgHandler2 WITH SA_NODEFER");
	sigact.sa_flags = SA_NODEFER;
	sigaction(SIGURG, &sigact, NULL);
	puts("Raising SIGURG in...3");
	sleep(2);
	puts("...2");
	sleep(2);
	puts("...1!");
	raise(SIGURG);
	
	exit(EXIT_SUCCESS);
}
