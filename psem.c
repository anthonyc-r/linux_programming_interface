/**
 * An implementation of posix semaphores using sysv semaphores.
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>

#define SEM_FAILED 0

typedef struct _sem {
	int id;
	char *path;
} sem_t;

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

	return semop(sem->id, &sop, 1);
}

int sem_wait(sem_t *sem) {
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = 0;

	return semop(sem->id, &sop, 1);
}

int sem_init(sem_t *sem, int pshared, unsigned int value) {
	int id;
	union semun arg;

	arg.val = value;
	id = semget(IPC_PRIVATE, 1, IPC_CREAT | S_IRWXU | S_IRWXO | S_IRWXG);
	if (id == -1)
		return -1;
	if (semctl(id, 0, SETVAL, arg) == -1)
		return -1;
	sem->id = id;
	sem->path = NULL;
	return 0;
}

sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value) {
	int fd;
	sem_t *sem;
	union semun arg;

	if ((sem->path = malloc(PATH_MAX)) == NULL)
		return (sem_t*)SEM_FAILED;
	if (snprintf(sem->path, PATH_MAX, "/tmp/sem_%s", name) < 0)
		return (sem_t*)SEM_FAILED;
	// This should hand'e all the O_EXCL/CREAT details
	if ((fd = open(sem->path, oflag)) == -1)
		return (sem_t*)SEM_FAILED;
	if (close(fd) == -1)
		return (sem_t*)SEM_FAILED;
	if ((sem->id = semget(ftok(sem->path, 1), 1, IPC_CREAT | oflag)) == -1)
		return (sem_t*)SEM_FAILED;
	arg.val = value;
	if (semctl(sem->id, 0, SETVAL, arg) == -1)
		return (sem_t*)SEM_FAILED;
	return sem;
}

int sem_close(sem_t *sem) {
	free(sem);
	return 0;
}

int sem_unlink(sem_t *sem) {
	union semun arg;

	if (unlink(sem->path) == -1)
		return -1;
	if (semctl(sem->id, 0, IPC_RMID, arg) == -1)
		return -1;
	return 0;
}

int sem_destroy(sem_t *sem) {
	union semun arg;

	return semctl(sem->id, 0, IPC_RMID, arg);
}

// Test the above
static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	sem_t sem;

	puts("init value 1");
	if (sem_init(&sem, 0, 1) == -1)
		exit_err("sem_init");
	puts("wait");
	if (sem_wait(&sem) == -1)
		exit_err("sem_wait");

	puts("fork");
	switch (fork()) {
	case -1:
		exit_err("fork");
	case 0:
		sleep(1);
		puts("(child) post");
		sem_post(&sem);
		_exit(EXIT_SUCCESS);
	default:
		break;
	}
	puts("(parent) wait");
	if (sem_wait(&sem) == -1)
		exit_err("sem_wait2");
	puts("(parent) done");

	puts("Test named sem impl");
 	exit(EXIT_SUCCESS);
}
