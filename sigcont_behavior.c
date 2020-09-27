/**
* SIGCONT resumes a stopped process even when the signal is blocked
* However, a custom signal handler will only get called after the
* signal has been unblocked. This program verifies that behavior.
*
* In order to verify behavior - run this program, stop it (ctl-z), then use
* The fg command or kill SIGCONT. Noting that nothing is printed. 
* send a SIGINT (ctl-c) after resuming, which will unblock SIGCONT, noting
* only then is the signal handler called (it prints out).
*/
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t finished;

// Non async signal safe stdio functions.

static void cont_handler(int sig) {
	puts("SIGCONT handler called!");
}

static void int_handler(int sig) {
	sigset_t block;
	puts("INT received, unblocking CONT");
	sigemptyset(&block);
	sigprocmask(SIG_SETMASK, &block, NULL);
	finished = 1;
}

int main(int argc, char **argv) {
	struct sigaction sigact;
	sigset_t block;
	
	sigemptyset(&block);
	sigaddset(&block, SIGCONT);
	sigprocmask(SIG_SETMASK, &block, NULL);
	
	/** 
	* Memseting this whole struct to 0 (as done in some previous files)
	* isn't guarenteed to work, as there's no requirements on how the 
	* sigset is implemented. (Maybe 0 doesn't represent no signals??)
	* Hence sigempty set is used properly here.
	*/
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_handler = cont_handler;
	sigaction(SIGCONT, &sigact, NULL);
	
	sigact.sa_handler = int_handler;
	sigaction(SIGINT, &sigact, NULL);
	
	while(!finished) {
		sleep(1);
	}
	
	exit(EXIT_SUCCESS);
}
