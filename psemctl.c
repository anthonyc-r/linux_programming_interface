/**
* psemctl - ./semclt rewritten for posix semaphores
* semctl [-ch] [-t time] [-n name] command
* Where command is one of...
* create, wait, post, remove
*
* Options:
* -n name: The name of the semaphore in the usual /path format.
* -c: Performs the operation conditionally. If the operation would block
*	then the program will exit indicating an error.
* -t time: Applicable only to the wait command, when specified performs a
*	timed wait. After 'time' seconds, the wait operation will timeout.
*/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>

static void print_usage() {
	puts("psemctl - ./semclt rewritten for posix semaphores");
	puts(" semctl [-ch] [-t time] [-n name] command");
	puts(" Where command is one of...");
	puts(" create, wait, post, remove");
	puts("");
	puts(" Options:");
	puts(" -n name: The name of the semaphore in the usual /path format.");
	puts(" -c: Performs the operation conditionally. If the operation would block");
	puts("	then the program will exit indicating an error.");
	puts(" -t time: Applicable only to the wait command, when specified performs a");
	puts("	timed wait. After 'time' seconds, the wait operation will timeout.");
	exit(EXIT_FAILURE);
}

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static struct timespec *get_ts(char *arg, struct timespec *ts) {
	char *ep;
	long t;

	t = strtol(arg, &ep, 10);
	if (ep == arg || *ep != '\0')
		return NULL;
	clock_gettime(CLOCK_REALTIME, ts);
	ts->tv_sec += t;
	return ts;
}


static void screate(char *name) {
	int oflag, perms, value;

	oflag = O_RDONLY | O_CREAT | O_EXCL;
	perms = S_IRWXU | S_IRWXO;
	value = 0;
	if (sem_open(name, oflag, perms, value) == SEM_FAILED)
		exit_err("sem_open excl");
}

static void swait(char *name, int cond, struct timespec *ts) {
	sem_t *sem;

	if ((sem = sem_open(name, O_RDONLY)) == SEM_FAILED)
		exit_err("sem_open");
	if (ts != NULL) {
		if (sem_timedwait(sem, ts) == -1)
			exit_err("sem_timedwait");
	} else if (cond) {
		if (sem_trywait(sem) == -1)
			exit_err("sem_trywait");
	} else {
		if (sem_wait(sem) == -1)
			exit_err("sem_wait");
	}
}

static void spost(char *name) {
	sem_t *sem;

	if ((sem = sem_open(name, O_WRONLY)) == SEM_FAILED)
		exit_err("sem_open");
	if (sem_post(sem) == -1)
		exit_err("sem_post");
}

static void sremove(char *name) {
	if (sem_unlink(name) == -1)
		exit_err("sem_unlink");
}

int main(int argc, char **argv) {
	int cond, hold, opt;
	char *name;
	struct timespec *tsp, ts;

	cond = 0;
	hold = 0;
	name = NULL;
	tsp = NULL;
	while ((opt = getopt(argc, argv, "cht:n:")) != -1) {
		switch (opt) {
		case 'c':
			cond = 1;
			break;
		case 'n':
			name = optarg;
			break;
		case 't':
			tsp = get_ts(optarg, &ts);
			break;
		case 'h':
			hold = 1;
			break;
		case '?':
			print_usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		print_usage();
	if (strcmp(argv[0], "create") == 0) {
		if (name == NULL)
			print_usage();
		screate(name);
	} else if (strcmp(argv[0], "wait") == 0) {
		if (name == NULL)
			print_usage();
		swait(name, cond, tsp);
	} else if (strcmp(argv[0], "post") == 0) {
		if (name == NULL)
			print_usage();
		spost(name);
	} else if (strcmp(argv[0], "remove") == 0) {
		if (name == NULL)
			print_usage();
		sremove(name);
	} else {
		print_usage();
	}
	puts("OK");
	if (hold) while (1) pause();

	exit(EXIT_SUCCESS);
}
