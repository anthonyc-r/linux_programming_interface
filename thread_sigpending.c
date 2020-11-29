/*
* Demonstrate that different threads can have different sets of pending signals
* Also appears that signals sent to self via raise() are also specific, rather than process
* wide...
*/

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

struct targ {
	char *name;
	int sig;
};

static void wait_int() {
	int sig;
	sigset_t waitset;
	sigaddset(&waitset, SIGINT);
	sigwait(&waitset, &sig);
}

static void ppending(char *tname) {
	sigset_t pending;
	sigpending(&pending);
	if (sigismember(&pending, SIGUSR1)) {
		printf("%s: USR1 pending!\n", tname);
	}
	if (sigismember(&pending, SIGUSR2)) {
		printf("%s: USR2 pending!\n", tname);
	}
	if (sigismember(&pending, SIGQUIT)) {
		printf("%s: QUIT pending!\n", tname);
	}
}

static void *fn(void *arg) {
	struct targ *targ ;
	sigset_t unblock;

	targ = (struct targ*)arg;
	sigaddset(&unblock, targ->sig);
	pthread_sigmask(SIG_UNBLOCK, &unblock, NULL);
	wait_int();
	ppending(targ->name);
	wait_int();
	return NULL;
}


static void err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	pthread_t t1, t2;
	sigset_t block;
	sigaddset(&block, SIGINT);
	sigaddset(&block, SIGUSR1);
	sigaddset(&block, SIGUSR2);
	sigaddset(&block, SIGQUIT);
	sigaddset(&block, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &block, NULL);

	struct targ targs[2] = {
		{"thread1", SIGUSR2},
		{"thread2", SIGUSR1}
	};
	
	if (pthread_create(&t1, NULL, fn, (void*)targs) != 0)
		err("pthread_create");
	if (pthread_create(&t2, NULL, fn, (void*)(targs + 1)) != 0)
		err("pthread_create");
	sigdelset(&block, SIGQUIT);
	sigdelset(&block, SIGALRM);
	pthread_sigmask(SIG_UNBLOCK, &block, NULL);
	
	// Thread specific....?!
	raise(SIGQUIT);
	alarm(1);
	// Thread specific.
	pthread_kill(t1, SIGUSR1);
	pthread_kill(t2, SIGUSR2);
	
	sleep(2);
	// Print info.
	pthread_kill(t1, SIGINT);
	pthread_kill(t2, SIGINT);

	sleep(1);
	ppending("main");

	exit(EXIT_SUCCESS);
}