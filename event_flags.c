/**
* An implementation of event flags found in VMS using sysV semaphores.
* A flag is either clear or set.
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>

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


int initEventFlag(int semid, int semn) {
	return 0;
}

int setEventFlag(int semid, int semn) {
	int semv, res;
	union semun arg;
	
	arg.val = 0;
	return semctl(semid, semn, SETVAL, arg);
}

int clearEventFlag(int semid, int semn) {
	int semv, res;
	union semun arg;
	
	arg.val = 1;
	return semctl(semid, semn, SETVAL, arg);;
}

int waitForEventFlag(int semid, int semn) {
	struct sembuf sop;
	sop.sem_num = semn;
	sop.sem_op = 0;
	sop.sem_flg = 0;
	
	return semop(semid, &sop, 1);
}

int getFlagState(int semid, int semn) {
	int semv, res;
	union semun arg;
	
	res = semctl(semid, semn, GETVAL, arg);
	if (res == -1)
		return -1;
	else
		return arg.val == 0 ? 1 : 0;
}

static void handler(int sig) {
	// Nothing.
}


// Test the impl out.
int main(int argc, char **argv) {
	int semid;
	union semun arg;
	
	// Setup the semaphore...
	if ((semid = semget(IPC_PRIVATE, 1, S_IRWXU)) == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}
	arg.val = 1;
	if (semctl(semid, 0, SETVAL, arg) == -1) {
		perror("semctl");
		exit(EXIT_FAILURE);
	}
		
	// Give it a go.
	switch (fork()) {
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
	case 0:
		puts("[Child] Sleeping...");
		sleep(1);
		puts("[Child] Set event flag!");
		setEventFlag(semid, 0);
		_exit(EXIT_SUCCESS);
	default:
		puts("[Parent] Wait for event flag...");
		waitForEventFlag(semid, 0);
		puts("Parent] Event flag set!");
		break;
	}
	
	puts("Wait for flag already set!");
	waitForEventFlag(semid, 0);
	puts("Continue. Clear & Wait on flag with 1s timeout...");
	clearEventFlag(semid, 0);
	signal(SIGALRM, handler);
	alarm(1);
	waitForEventFlag(semid, 0);
	puts("Timed out. Flag state?");
	if (getFlagState(semid, 0)) {
		puts("Flag set");
	} else {
		puts("Flag not set");
	}
	puts("Set flag. Flag state?");
	setEventFlag(semid, 0);
	if (getFlagState(semid, 0)) {
		puts("Flag set");
	} else {
		puts("Flag not set");
	}
	puts("Finished tests!");
	
	semctl(semid, 0, IPC_RMID, arg);
	exit(EXIT_SUCCESS);
}
