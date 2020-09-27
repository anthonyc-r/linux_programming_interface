/**
* Realtime signals are delivered in ascending order, so their integer value is
* like a priority level. However, it is not standardized as to whether standard
* signals are delivered before realtime signals, or vice versa.
* This program demonstrates whether realtime or standard signals are delivered
* first on the host system. (Actually OpenBSD at the time of writing).
*
* This program first blocks all signals, during which time the user should send
* various realtime and standard signals to this process. Afterwards, send a
* SIGINT to unblock all signals. The program will print out signals as they are
* received, and then terminate. SIGQUIT will terminate the program.
*/

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>

static volatile sig_atomic_t finished;

// stdio functions are not asycn signal safe.
static void int_handler(int sig) {
	puts("Unblocking all signals");
	sigset_t block;
	sigemptyset(&block);
	sigprocmask(SIG_SETMASK, &block, NULL);
}

static void quit_handler(int sig) {
	puts("Finishing");
	finished = 1;
}

// psignal is not safe either, but for expediency!
static void all_handler(int sig) {
	psignal(sig, "Received signal");
}

int main(int argc, char **argv) {
	struct sigaction sigact;
	sigset_t block;
	int sig;
	
	sigfillset(&block);
	sigdelset(&block, SIGINT);
	sigdelset(&block, SIGQUIT);
	
	sigprocmask(SIG_SETMASK, &block, NULL);
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	
	sigact.sa_handler = all_handler;
	for (sig = 1; sig < NSIG; sig++) {
		sigaction(sig, &sigact, NULL);
	} 
	
	sigact.sa_handler = int_handler;
	sigaction(SIGINT, &sigact, NULL);
	sigact.sa_handler = quit_handler;
	sigaction(SIGQUIT, &sigact, NULL);
	
	printf("Realtime signals are from %d to %d\n", SIGRTMIN, SIGRTMAX);
	while (!finished) {
		sleep(1);
	}
	exit(EXIT_SUCCESS);
}
