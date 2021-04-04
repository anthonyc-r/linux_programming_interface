/**
* semctl - A command line util for managing sysv semaphores   
* semctl [-uc] [-i semid] command [arg]
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
*/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define _NO_SEMID -1
#define _NO_SEMOP 0x80000000

static void print_usage() {
	puts(" semctl - A command line util for managing sysv semaphores");
	puts(" semctl [-uc] [-i semid] command [op]");
	puts(" Where command is one of...");
	puts(" create, apply, list, remove");
	puts(" ");
	puts(" When the command is apply, then op is the operation to perform. e.g. +2.");
	puts(" ");
	puts(" Options:");
	puts(" -i semid: The semaphore id on which to operate where applicable.");
	puts(" -u: Applys the SEM_UNDO flag when creating the semaphore.");
	puts(" -c: Performs the operation conditionally. If the operation would block");
	puts(" 	then the program will exit indicating an error.");
	puts("");
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
	puts("create");
}

static void sem_apply(int semid, int semop) {
	puts("apply");
}

static void sem_list() {
	puts("list");
}

static void sem_remove(int semid) {
	puts("remove");
}

int main(int argc, char **argv) {
	int undo, cond, semid, semop, opt;
	
	undo = 0;
	cond = 0;
	semid = _NO_SEMID;
	while ((opt = getopt(argc, argv, "uci:")) != -1) {
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
		if ((semop = getnum(optarg, _NO_SEMOP)) == _NO_SEMOP)
			print_usage();
		sem_apply(semid, semop);
	} else if (strcmp(argv[0], "remove") == 0) {
		if (semid == _NO_SEMID)
			print_usage();
		sem_remove(semid);
	} else if (strcmp(argv[0], "list") == 0) {
		sem_list();
	} else {
		print_usage();
	}
	
	
	exit(EXIT_SUCCESS);
}
