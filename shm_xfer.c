#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h> 

#define SHM_FILE "/tmp/shm_xfer.shm"
#define BLOCK_SZ 1024

struct xfer_buf {
	size_t n;
	char buf[BLOCK_SZ];
};

static int exit_usage() {
	puts("usage: shm_xfer (send|receive)");
	exit(EXIT_FAILURE);
}

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void log(char *msg) {
	puts(msg);
}

static void receive() {
	struct stat sb;
	int shmid, semid, fd, lastn;
	struct shmid_ds shmds;
	struct semid_ds semds;
	union semun semarg;
	semarg.buf = &semds;
	struct sembuf sop;
	struct xfer_buf *xbuf;
	char buf[BLOCK_SZ];
	
	if (stat(SHM_FILE, &sb) == -1) {
		printf("File doesn't exit (%s) - nothing being sent to receive\n",
			SHM_FILE);
		exit(EXIT_FAILURE);
	}
	if ((fd = open(SHM_FILE, O_RDONLY)) == -1)
		exit_err("Failed to open shm file");
	if (read(fd, &shmid, sizeof (int)) != sizeof (int))
		exit_err("Invalid shm file");
	if (read(fd, &semid, sizeof (int)) != sizeof (int))
		exit_err("Invalid shm file");
	if (shmctl(shmid, IPC_STAT, &shmds) == -1)
		exit_err("Invalid shmid in file");
	if (semctl(semid, 0, IPC_STAT, semarg) == -1)
		exit_err("Invalid semid in file");
	if (close(fd) == -1)
		exit_err("close SHM_FILE");
	if ((xbuf = shmat(shmid, 0, 0)) == NULL)
		exit_err("shmat");
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	log("Tell server client is ready");
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop ready");
		
		
	while (1) {
		log("Take 1, client is reading");
		sop.sem_op = -1;
		if (semop(semid, &sop, 1) == -1)
			exit_err("semop wait for read");
		
		sleep(1);
		if ((lastn = xbuf->n) < 1) {
			log("xbuf->n < 1. Take 1, client finished reading");
			if (semop(semid, &sop, 1) == -1)
				exit_err("semop indicate read");
				break;
		}
		if (write(STDOUT_FILENO, &xbuf->buf, xbuf->n) != xbuf->n)
			exit_err("write stdout");
			
		log("Take 1, client finished reading");
		if (semop(semid, &sop, 1) == -1)
			exit_err("semop indicate read");
	}
	log("xbuf->n < 1. Detatch.");
	sleep(1);	
	shmdt(xbuf);
	log("Tell server we've cleaned up");
	sop.sem_op = 1;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop cleaned up");
	if (lastn != 0)
		exit_err("Non 0 final status");
}

static void send() {
	int fd, shmid, semid;
	struct stat sb;
	struct xfer_buf *xbuf;
	char buf[BLOCK_SZ];
	ssize_t nread;
	union semun semarg;
	struct sembuf sop; 
	
	if (stat(SHM_FILE, &sb) != -1) {
		printf("File exists (%s) - transfer already in progress?\n",
			SHM_FILE);
		exit(EXIT_FAILURE);
	}
	shmid = shmget(IPC_PRIVATE, sizeof (struct xfer_buf), O_WRONLY 
		| S_IWUSR | S_IRUSR);
	if (shmid == -1)
		exit_err("shmget");
	if ((semid = semget(IPC_PRIVATE, 1, O_RDWR | S_IRWXU)) == -1)
		exit_err("semget");
	
	// Init the semaphore at 0./
	semarg.val = 1;
	if (semctl(semid, 0, SETVAL, semarg) == -1)
		exit_err("semctl init");
	
	if ((fd = open(SHM_FILE, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR)) == -1)
		exit_err("open SHM_FILE");
	if (write(fd, &shmid, sizeof (int)) != sizeof (int))
		exit_err("write shmid SHM_FILE");
	if (write(fd, &semid, sizeof (int)) != sizeof (int))
		exit_err("write semid SHM_FILE");
	if (close(fd) == -1)
		exit_err("close SHM_FILE");
	if ((xbuf = shmat(shmid, 0, 0)) == NULL)
		exit_err("shmat");
	

	log("Waiting for a client..");
	sop.sem_num = 0;
	sop.sem_op = 0;
	sop.sem_flg = 0;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop wait for client to begin");
	
	unlink(SHM_FILE);
	while ((nread = read(STDIN_FILENO, buf, BLOCK_SZ)) > 0)  {
		sleep(1);
		log("Writing to shared memory");
		xbuf->n = nread;
		memcpy(xbuf->buf, buf, nread);
		log("Add 2. Tell client it can read");
		sop.sem_op = 2;
		// Tell client to read...
		if (semop(semid, &sop, 1) == -1)
			exit_err("semop tell client to read...");
		log("Wait for client to finish reading (0).");
		sop.sem_op = 0;
		if (semop(semid, &sop, 1) == -1)
			exit_err("semop wait to write...");
	}
	sleep(1);
	if (nread == -1) {
		xbuf->n = -1;
	} else {
		// Indicate eof.
		xbuf->n = 0;
	}
	log("Tell client it can read (for final time)...");
	sop.sem_op = 2;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop last read");
	sop.sem_op = 0;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop wait for last read");
	sleep(1);
	log("Wait for client to cleanup");
	sop.sem_op = 0;
	if (semop(semid, &sop, 1) == -1)
		exit_err("semop wait for client cleanup");
	
	log("OK. Remove IPC objects");
	if (shmdt(xbuf))
		exit_err("shmdt");
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		exit_err("shmctl rmid");
	if (semctl(semid, 0, IPC_RMID, semarg) == -1)
		exit_err("semctl rmid");
	
}

int main(int argc, char **argv) {
	if (argc < 2)
		exit_usage();
		
	if (strcmp("receive", argv[1]) == 0)
		receive();
	else if (strcmp("send", argv[1]) == 0)
		send();
	else 
		exit_usage();
	exit(EXIT_SUCCESS);
}
