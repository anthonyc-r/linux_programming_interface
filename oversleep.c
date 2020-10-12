/*
* Use in conjuction with repeated interupts, like so:
* "while true; do kill -INT <this pid>; done
* in order to demonstrate the increase in sleep time.
* 
* This is because nanosleep's resolution is limited to that of the software 
* clock, and it rounds UP to the nearest unit of the software clock. So on
* every interrupt, the time remaining gets rounded up.
*
* Linux provides a non-portable clock_nanosleep, which allows us to sleep 
* based off of the 3 interval clocks, along with the TIME_ABSTIME flag, 
* this lets us sleep based of of an absolute time in the future, 
* not a relative amount.
* compile with -DABSTIME to test this.
*/
#define _POSIX_C_SOURCE 199309
 
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void handler(int sig) {
	// Do nothing...
}

#ifdef ABSTIME
void clock_nsleep() {
	struct timespec now;
	int err;

	printf("Performing sleep with clock_nanosleep!\n");
	clock_gettime(CLOCK_REALTIME, &now);
	now.tv_sec += 10;
	while (1) {
		if ((err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, 
			&now, NULL)) == 0) {
			break;
		}
		if (err != EINTR) {
			printf("clock_nanosleep\n");
			exit(EXIT_FAILURE);
		}
		printf("Interrupted.\n");
	}
}
#endif

void nsleep() {
	struct timespec current_sleep, remaining;

	printf("Performing sleep with plain nanosleep\n");
	current_sleep.tv_sec = 10;
	current_sleep.tv_nsec = 0;
	while (1) {
		if (nanosleep(&current_sleep, &remaining) != -1) {
			break;
		}
		if (errno != EINTR) {
			printf("nanosleep\n");
			exit(EXIT_FAILURE);
		}
		printf("Interrupted\n");
		current_sleep = remaining;
	}
}

int main(int argc, char **argv) {
        struct sigaction sigact;
	time_t start;	

        printf("pid: %d\n", getpid());

        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags = 0;
        sigact.sa_handler = handler;
        sigaction(SIGINT, &sigact, NULL);

	start = time(NULL);
	#ifdef ABSTIME 
		clock_nsleep();
	#else
		nsleep();
	#endif
	printf("Slept for %lld seconds!\n", time(NULL) - start);
}
