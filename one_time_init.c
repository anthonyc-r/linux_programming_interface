/*
* Implement a function similar to pthread_once
*/

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct control {
	pthread_mutex_t mut;
	bool fired;
} control_t;

#define CONTROL_INIT { PTHREAD_MUTEX_INITIALIZER, false };

int one_time_init(control_t *control, void (*init)(void)) {
	int err;
	if((err = pthread_mutex_lock(&control->mut)) != 0)
		return err;

	if (!control->fired) {
		control->fired = true;
		if ((err = pthread_mutex_unlock(&control->mut)) != 0)
			return err;
		init();
	} else {
		if ((err = pthread_mutex_unlock(&control->mut)) != 0)
			return err;
	}
	return 0;
}

/* Test Code */
static control_t phello_control = CONTROL_INIT;
static control_t pworld_control = CONTROL_INIT;

static void phello() {
	printf("Hellooo\n");
}

static void pworld() {
	printf("World!!\n");
}

void *phello_fn(void *arg) {
	one_time_init(&phello_control, phello);
	return (void*)0;
}

void *pworld_fn(void *arg) {
	one_time_init(&pworld_control, pworld);
	return (void*)0;
}

int main(int argc, char **argv) {
	int i;
	pthread_t thread;

	for (i = 0; i < 100; i++) {
		if (i % 2) {
			pthread_create(&thread, NULL, phello_fn, NULL);
		} else {
			pthread_create(&thread, NULL, pworld_fn, NULL);
		}
	}

	pthread_exit((void*)0);
}