/**
* semctl - A command line util for managing sysv semaphores   
* semctl [-uch] [-i semid] command [arg]
* Where command is one of...
* create, apply, list, remove
* 
* When the command is apply, then args is the operation to perform. e.g. +2.
*
* Options:
* -i semid: The semaphore id on which to operate where applicable.
* -u: Applys the SEM_UNDO flag when creating the semaphore.
* -c: Performs the operation conditionally. If the operation would block
*	then the program will exit indicating an error.
* -h: Hold; Keep the program running until it receives a signal
*/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>

#define _NO_SEMID -1
#define _NO_SEMOP 0x80000000

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
#if defined(__linuix__)
	struct seminfo *__buf;
#endif
};
#endif


static void print_usage() {
	puts(" semctl - A command line util for managing sysv semaphores");
	puts(" semctl [-uch] [-i semid] command [op]");
	puts(" Where command is one of...");
	puts(" create, apply, list, remove");
	puts(" ");
	puts(" When the command is apply, then op is the operation to perform. e.g. +2.");
	puts(" ");
	puts(" Options:");
	puts(" -i semid: The semaphore id on which to operate where applicable.");
	puts(" -u: Applys the SEM_UNDO flag when creating the semaphore.");
	puts(" -c: Performs the operation conditionally. If the operation would block");
	puts(" -h: Hold; Keep the program running until it receives a signal");
	puts(" 	then the program will exit indicating an error.");
	puts("");
	exit(EXIT_FAILURE);
}

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static int getnum(char *arg, int dfl) {
	char *ep;
	int semid;
	
	semid = (int)strtol(arg, &ep, 10);
	if (*ep != '\0')
		return dfl;
	else
		return semid;
}


static void sem_create() {
	int semid;
	
	// Accepts same sort of flags as open, but O_NONBLOCK doesn't have effect?
	if ((semid = semget(IPC_PRIVATE, 1, S_IRWXU)) == -1)
		exit_err("semget");
	printf("%d\n", semid);
}

static void sem_apply(int semid, int sem_op, int cond, int undo) {
	struct sembuf sop;
	
	sop.sem_op = sem_op;
	sop.sem_num = 0;
	sop.sem_flg = 0;
	if (cond)
		sop.sem_flg |= IPC_NOWAIT;
	if (undo)
		sop.sem_flg |= SEM_UNDO;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop");
}

static void sem_list() {
	union semun arg;
	struct semid_ds buf;
	int i;
	
	arg.buf = &buf;
	// TODO: - Upper bound?
	for (i = 0; i < 1000000; i++) {
		if (semctl(i, 0, IPC_STAT, arg) != -1)
			// TODO: - Detailed info.
			printf("%08x\n", i);
	}
}

static void sem_remove(int semid) {
	union semun arg;
	
	if (semctl(semid, 0, IPC_RMID, arg) == -1)
		exit_err("semctl");
}

int main(int argc, char **argv) {
	int undo, cond, semid, semop, opt, hold;
	
	undo = 0;
	cond = 0;
	hold = 0;
	semid = _NO_SEMID;
	while ((opt = getopt(argc, argv, "uchi:")) != -1) {
		switch (opt) {
		case 'u':
			undo = 1;
			break;
		case 'c':
			cond = 1;
			break;
		case 'i':
			semid = getnum(optarg, _NO_SEMID);
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
		sem_create();
	} else if (strcmp(argv[0], "apply") == 0) {
		if (argc < 2 || semid == _NO_SEMID)
			print_usage();
		if ((semop = getnum(argv[1], _NO_SEMOP)) == _NO_SEMOP)
			print_usage();
		sem_apply(semid, semop, cond, undo);
	} else if (strcmp(argv[0], "remove") == 0) {
		if (semid == _NO_SEMID)
			print_usage();
		sem_remove(semid);
	} else if (strcmp(argv[0], "list") == 0) {
		sem_list();
	} else {
		print_usage();
	}
	
	if (hold) while (1) pause();
	
	exit(EXIT_SUCCESS);
}
