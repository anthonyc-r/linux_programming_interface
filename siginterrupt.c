/*
Implement siginterrupt() using sigaction.
if flag is 0 then blocking system calls will be restarted after handler execution
else if flag is 1 then the handler will interrupt blocking system calls.

This function is marked as obsolete in SUSv4 - and recommmends using sigaction with SA_RESTART.
*/
#include <signal.h>
#include <stdlib.h>

int my_siginterrupt(int sig, int flag) {
	struct sigaction sigact;
	if (sigaction(sig, NULL, &sigact) == -1)
		return -1;
	if (flag) 
		sigact.sa_flags ^= SA_RESTART;
	else
		sigact.sa_flags |= SA_RESTART;
	if (sigaction(sig, &sigact, NULL) == -1)
		return -1;
	return 0;
}

int main(int argc, char **argv) {
	my_siginterrupt(SIGUSR1, 1);	
	exit(EXIT_SUCCESS);
}