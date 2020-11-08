/*
* Each number argument provided creates a thread which increments a global variable by the 
* number of times equal to said number.
* The inner loop is protected by a pthread_mutex so as to prevent race conditions.
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// Protects the glob variable.
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int glob;

struct thread_info {
	pthread_t thread;
	int id;
	int count;
} *thread_info;

static void exit_usage(char *argv0) {
	printf("Usage: %s num1 [num2 [num3 ...]]\n", argv0);
	exit(EXIT_FAILURE);
}

static void *thread_fun(void *arg) {
	int i, loc;
	struct thread_info *info = (struct thread_info *)arg;

	for (i = 0; i < info->count; i++) {
		pthread_mutex_lock(&mtx);
		// Even ++ isn't an atomic operation on many RISC architectures.
		loc = glob;
		loc += 1;
		glob = loc;
		printf("[Thread %02d] Incremeneted to %d\n", info->id, glob);
		pthread_mutex_unlock(&mtx);
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	int i;
	char *ep;

	if (argc < 2)
		exit_usage(argv[0]);
	thread_info = calloc(argc - 1, sizeof (struct thread_info));
	if (thread_info == NULL) {
		printf("calloc\n");
		exit(EXIT_FAILURE);
	}
	for (i = 1; i < argc; i++) {
		thread_info[i].id = i;
		thread_info[i].count = (int)strtol(argv[i], &ep, 10);
		if (*ep != '\0') {
			printf("This doesn't look like a number... (%s)\n", argv[i]);
			exit_usage(argv[0]);
		}
		pthread_create(&thread_info[i].thread, NULL, thread_fun, &thread_info[i]);
	}

	pthread_exit(NULL);
}