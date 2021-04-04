/* Synchronize child and parent processes using a sysv semaphore. */

#import <unistd.h>
#import <stdlib.h>
#import <sys/stat.h>
#import <sys/types.h>
#import <sys/sem.h>
#import <stdio.h>
#import <string.h>
#import <errno.h>

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

static int semid;

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	semctl(semid, 0, IPC_RMID);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	pid_t child;
	union semun semun;
	struct sembuf sop;
	
	if ((semid = semget(IPC_PRIVATE, 1, S_IRWXU)) == -1)
		exit_err("semget");
	semun.val = 0;
	if (semctl(semid, 0, SETVAL, semun) == -1)
		exit_err("semct setval");
 	
	sop.sem_num = 0;
 	sop.sem_flg = 0;
	switch ((child = fork())) {
	case -1:
		exit_err("fork");
	case 0:
		sop.sem_op = -1;
		puts("Child waiting...");
		if (semop(semid, &sop, 1) == -1) {
			puts("child semop err");
			_exit(EXIT_FAILURE);
		}
		puts("Child continues...");
		_exit(EXIT_SUCCESS);
	default:
		break;
	}
	
	puts("Parent doing some work... hold sem");
	sleep(1);
	sop.sem_op = 1;
	puts("Parent done, release sem");
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop");

	
	if (semctl(semid, 0, IPC_RMID) == -1)
		exit_err("semctl rmid");
	exit(EXIT_SUCCESS);
}
