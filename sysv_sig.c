/**
* An implementation of the system V functions sigset() sighold() sigrelse()
* and sigpause()
*/
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// dfl, ign and error are 0, 1, -1.
#define SIG_HOLD (void (*)(int))2

// Returns the previous disposition of sig, or SIG_HOLD if sig was
// previously blocked, or SIG_ERR on error.
void (*sigset(int sig, void (*handler)(int)))(int) {
	struct sigaction sigact;
	sigset_t mask;
	int blocked;
	void (*prev_handler)(int);

	if (sigprocmask(SIG_BLOCK, NULL, &mask) == -1)
		return SIG_ERR;
	if ((blocked = sigismember(&mask, sig)) == -1)
		return SIG_ERR;
	if (handler == SIG_HOLD && blocked)
		return SIG_HOLD;
	
	if (sigaction(sig, NULL, &sigact) == -1)
		return SIG_ERR;
	prev_handler = sigact.sa_handler;
	if (blocked)
		prev_handler = SIG_HOLD;
		
	if (sigemptyset(&mask) == -1)
		return SIG_ERR;
	if (sigaddset(&mask, sig) == -1)
		return SIG_ERR;
		
	if (handler == SIG_HOLD) {
		if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
			return SIG_ERR;
	} else {
		if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
			return SIG_ERR;
		if (sigemptyset(&sigact.sa_mask) == -1)
			return  SIG_ERR;
		sigact.sa_flags = 0;
		sigact.sa_handler = handler;
		if (sigaction(sig, &sigact, NULL) == -1)
			return  SIG_ERR;
	}
	
	return prev_handler;
}

// Add signal to the process mask
int sighold(int sig) {
	sigset_t set;
	if (sigemptyset(&set) == -1)
		return -1;
	if (sigaddset(&set, sig) == -1)
		return -1;
	return sigprocmask(SIG_BLOCK, &set, NULL);
}

// Remove signal from the process mask
int sigrelse(int sig) {
	sigset_t set;
	if (sigemptyset(&set) == -1)
		return  -1;
	if (sigaddset(&set, sig) == -1)
		return -1;
	return sigprocmask(SIG_UNBLOCK, &set, NULL);
}

// Sets the signals disposition to ignore
int sigignore(int sig) {
	struct sigaction sigact;
	if (sigemptyset(&sigact.sa_mask) == -1)
		return -1;
	sigact.sa_flags = 0;
	sigact.sa_handler = SIG_IGN;
	return sigaction(sig, &sigact, NULL);
}

// Atomically remove a signal from the process mask and suspend execution until
// signal arrival.
int sigpause(int sig) {
	sigset_t set;
	if(sigprocmask(SIG_BLOCK, NULL, &set) == -1)
		return -1;
	if (sigdelset(&set, sig) == -1)
		return -1;
	return sigsuspend(&set);
}

// Quick test!

void handler(int sig) {
	psignal(sig, "Signal received");
}

int main(int argc, char **argv) {
	sigset(SIGINT, handler);
	puts("Holding SIGINT and sleeping for 5 - try and interrupt me!");
	sighold(SIGINT);
	sleep(5);
	puts("sigpause SIGINT");
	sigpause(SIGINT);
	puts("Done!");
	exit(EXIT_SUCCESS);
}
