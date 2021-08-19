/**
 * An implementation of posix semaphores using sysv semaphores.
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/stat.h>

typedef int sem_t;

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
#if defined(__linux__)
	struct seminfo *__buf;
#endif
};


int sem_post(sem_t *sem) {
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = 1;
	sop.sem_flg = 0;

	return semop(*sem, &sop, 1);
}

int sem_wait(sem_t *sem) {
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = 0;

	return semop(*sem, &sop, 1);
}

int sem_init(sem_t *sem, int pshared, unsigned int value) {
	int id;
	union semun arg;

	id = semget(IPC_PRIVATE, 1, IPC_CREAT | S_IRWXU | S_IRWXO | S_IRWXG);
	if (id == -1)
		return -1;
	if (semctl(id, 0, SETVAL, arg) == -1)
		return -1;
	*sem = id;
	return 0;
}

int main(int argc, char **argv) {
	sem_t sem;


    exit(EXIT_SUCCESS);
}
