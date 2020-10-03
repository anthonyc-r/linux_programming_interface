/**
* Realtime signals are delivered in ascending order, so their integer value is
* like a priority level. However, it is not standardized as to whether standard
* signals are delivered before realtime signals, or vice versa.
* This program demonstrates whether realtime or standard signals are delivered
* first on the host system.
*
* This program first blocks all signals, during which time the user should send
* various realtime and standard signals to this process. Afterwards, send a
* SIGINT to unblock all signals. The program will print out signals as they are
* received, and then terminate. SIGQUIT will terminate the program.
*
* Example session:
* $ ./realtime &
* [3] 58810
* ubuntu@ubuntu:~/workspace$ Realtime signals are from 34 to 64
* $ kill -USR1 58810 
* $ kill -USR1 58810 
* $ kill -USR1 58810 
* $ kill -USR2 58810 
* $ kill -USR2 58810 
* $ kill -USR2 58810 
* $ kill -USR2 58810 
* $ kill -35 58810 
* $ kill -37 58810 
* $ kill -40 58810 
* $ kill -44 58810 
* $ kill -INT 58810 
* Unblocking all signals
* Received signal: User defined signal 1
* $ Received signal: User defined signal 2
* Received signal: Unknown signal 35
* Received signal: Unknown signal 37
* Received signal: Unknown signal 40
* Received signal: Unknown signal 44
* kill -QUIT 58810 
* Finishing
*
* From the above it looks like standard signals are delivered before realtime.
* Note that if I were to leave sa_mask as empty, the above would be very
* confusingly reversed. This is because each signal handler gets interuppted by the
* next, before it reaches the print statement!
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
	
	// Note that if we leave this as empty set, we will appear to get
	// signals in reverse order! As each signal will get interupted by the
	// next, before the printf is reached in the sig handler!!
	sigfillset(&sigact.sa_mask);
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
