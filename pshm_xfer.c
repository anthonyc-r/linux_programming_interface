#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define SHM_NAME "/pshm_xfer_shm"
#define SEM_NAME "/pshm_xfer_sem"
#define BLOCK_SZ 1024

struct xfer_buf {
	size_t n;
	char buf[BLOCK_SZ];
};

static int exit_usage() {
	perror("usage: shm_xfer (send|receive)");
	exit(EXIT_FAILURE);
}

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void putslog(char *str) {
	fputs(str, stderr);
	fputs("\n", stderr);
}

static void receive() {
	int fd, lastn;
	struct xfer_buf *xbuf;
	char buf[BLOCK_SZ];
	sem_t *sem;

	if ((fd = shm_open(SHM_NAME, O_RDONLY, 0)) == -1)
		exit_err("Shared memory object doesn't exist. Probably nothing being sent to receive.");
	if ((xbuf = mmap(NULL, sizeof (struct xfer_buf), PROT_READ, MAP_SHARED, fd, 0)) == NULL)
		exit_err("Could not map shared memory object");
	if ((sem = sem_open(SEM_NAME, O_RDWR)) == (sem_t*)SEM_FAILED)
		exit_err("Could not open semaphore");
	putslog("Tell server client is ready");
	if (sem_post(sem) == -1)
		exit_err("sem post ready");

	while (1) {
		putslog("Wait for server to write");
		if (sem_wait(sem) == -1)
			exit_err("sem_wait");

		if ((lastn = xbuf->n) < 1) {
			putslog("xbuf->n < 1. Post, client finished reading");
			if (sem_post(sem) == -1)
				exit_err("semop indicate read");
			break;
		} else {
			if (write(STDOUT_FILENO, &xbuf->buf, xbuf->n) != xbuf->n)
				exit_err("write stdout");

			putslog("Post, client finished reading");
			if (sem_post(sem) == -1)
				exit_err("semop indicate read");
		}
	}
	putslog("xbuf->n < 1. Detatch.");
	sleep(1);
	munmap(xbuf, sizeof (struct xfer_buf));
	if (lastn != 0)
		exit_err("Non 0 final status");
}

static void send() {
	int fd;
	struct xfer_buf *xbuf;
	char buf[BLOCK_SZ];
	ssize_t nread;
	sem_t *sem;

	if ((sem = sem_open(SEM_NAME, O_CREAT | O_RDWR, S_IRWXU, 0)) == (sem_t*)SEM_FAILED)
		exit_err("sem_open");
	if ((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU)) == -1)
		exit_err("shm_open");
	if (ftruncate(fd, sizeof (struct xfer_buf)) == -1)
		exit_err("ftruncate");

	if ((xbuf = mmap(NULL, sizeof (struct xfer_buf), PROT_WRITE, MAP_SHARED, fd, 0)) == NULL)
		exit_err("mmap");


	putslog("Waiting for a client..");
	if (sem_wait(sem) == -1)
		exit_err("sem_wait for client to begin");

	shm_unlink(SHM_NAME);
	sem_unlink(SEM_NAME);
	while ((nread = read(STDIN_FILENO, buf, BLOCK_SZ)) > 0) {
		putslog("Writing to shared memory");
		xbuf->n = nread;
		memcpy(xbuf->buf, buf, nread);
		putslog("Tell client it can read..");
		if (sem_post(sem) == -1)
			exit_err("sem_post");
		putslog("Wait for client to finish reading (0).");
		if (sem_wait(sem) == -1)
			exit_err("sem_wait");
	}
	if (nread == -1) {
		xbuf->n = -1;
	} else {
		// Indicate eof.
		xbuf->n = 0;
	}
	putslog("Tell client it can read (for final time)...");
	if (sem_post(sem) == -1)
		exit_err("sem_post");

	munmap(xbuf, sizeof (struct xfer_buf));
	// Already unlinked shm object
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
