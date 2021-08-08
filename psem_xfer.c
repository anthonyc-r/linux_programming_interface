/**
* Data is transfered from stdin to stdout via two threads and a global buffer.
* stdin -> Thread 1 -> Global Buffer -> Thread 2 -> stdout
* Access to the global buffer is protected using a posix semaphore.
**/
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <semaphore.h>
#include <signal.h>

#define BUFSZ 512

static char buf[BUFSZ];
static ssize_t n;
static sem_t sem;


static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void *reader(void *arg) {
	puts("Reader running...");
	while (1) {
		if ((n = read(STDIN_FILENO, buf, BUFSZ)) == -1)
			exit_err("read");
		sem_post(&sem);
	}
	return (void*)0;
}

static void *writer(void *arg) {
	puts("Writer running...");
	while (1) {
		sem_wait(&sem);
		if (write(STDOUT_FILENO, buf, n) == -1)
			exit_err("write");
	}
	return (void*)0;
}

int main(int argc, char **argv) {
	pthread_t treader, twriter;
	void *ret;

	if (sem_init(&sem, 0, 0) == -1)
		exit_err("sem_init");
	if (pthread_create(&treader, NULL, reader, NULL) != 0)
		exit_err("pthread create 1");
	if (pthread_create(&twriter, NULL, writer, NULL) != 0)
		exit_err("pthread create 2");
	pthread_join(treader, &ret);
	pthread_join(twriter, &ret);
	exit(EXIT_SUCCESS);
}
