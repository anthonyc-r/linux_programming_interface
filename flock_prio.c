/**
 * Determine some basic rules about flock().
 * 1. Do shared locks have prio over excl, or vice versa?
 * 2. Are locks granted in FIFO order?
 *
 * There doesn't appear to be any particular priority; In each run of the following
 * program, locks are aquired without any pattern or order.
 * On some runs, the shared locks are acquired after the initial exclusive
 * lock is unlocked. On others any one of the exclusive locks are acquired.
**/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define FNAME "/tmp/flock_test.tmp"
#define UNLOCK_ALL -1
static pid_t pids[30];

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void catch(int sig) {
	// Nothing...
}

static void sh_lock(int arg) {
	int fd;
	sigset_t set;
	if ((fd = open(FNAME, O_RDWR)) == -1)
		exit_err("open");

	printf("[%d] Trying shared lock\n", (int)arg);
	if (flock(fd, LOCK_SH) == -1 && errno != EINTR)
		exit_err("flock LOCK_SH");
	printf("[%d] Shared lock aquired\n", (int)arg);
	pause();
	if (flock(fd, LOCK_UN) == -1)
		exit_err("flock LOCK_UN");
	printf("[%d] Shared lock released\n", (int)arg);
}

static void ex_lock(int arg) {
	int fd;
	if ((fd = open(FNAME, O_RDWR)) == -1)
		exit_err("open");

	printf("[%d] Trying exclusive lock\n", (int)arg);
	if (flock(fd, LOCK_EX) == -1 && errno != EINTR)
		exit_err("flock LOCK_EX");
	printf("[%d] Exclusive lock aquired\n", (int)arg);
	pause();
	if (flock(fd, LOCK_UN) == -1)
		exit_err("flock LOCK_UN");
	printf("[%d] Exclusive lock released\n", (int)arg);
}

static pid_t lock(int arg, int type) {
	pid_t pid;
	struct timespec ts;

	switch ((pid = fork())) {
		case -1:
			exit_err("fork");
		case 0:
			if (type == LOCK_SH)
				sh_lock(arg);
			else
				ex_lock(arg);
			exit(EXIT_SUCCESS);
		default:
			ts.tv_sec = 0;
			ts.tv_nsec = 500000000L;
			nanosleep(&ts, NULL);
			return pid;
	}
}

static void unlock(int idx) {
	struct timespec ts;
	pid_t pgrp;

	if (idx == UNLOCK_ALL) {
		if ((pgrp = getpgrp()) == -1)
			exit_err("getpgrp");
		kill(-pgrp, SIGINT);
	} else {
		kill(pids[idx], SIGINT);
	}
	ts.tv_sec = 0;
	ts.tv_nsec = 500000000L;
	nanosleep(&ts, NULL);
}


int main(int argc, char **argv) {
	int wstat, i, acc;
	struct sigaction sigact;

	sigact.sa_handler = catch;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sigact, NULL) == -1)
		exit_err("sigaction");
	setbuf(stdout, NULL);
	if (unlink(FNAME) == -1)
		exit_err("unlink");
	if (open(FNAME, O_CREAT | O_RDWR, S_IRWXU) == -1)
		exit_err("open");

	acc = 0;
	pids[acc] = lock(acc++, LOCK_EX);
	for (i = 0; i < 3; i++)
		pids[acc] = lock(acc++, LOCK_SH);
	pids[acc] = lock(acc++, LOCK_EX);
	for (i = 0; i < 4; i++)
		pids[acc] = lock(acc++, LOCK_EX);

	// 5 exclusive locks + some shared locks
	for (i = 0; i < 6; i++) {
		sleep(1);
		puts("UNLOCK_ALL");
		unlock(UNLOCK_ALL);
	}

	sleep(1);
	exit(EXIT_SUCCESS);
}
