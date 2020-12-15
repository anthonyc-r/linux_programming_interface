/**
* Demonstrate the behavior of the FIFO realtime scheduler.
* Set real time scheduler to FIFO, create a child process, consume 3 seconds CPU time. 
* Every 0.25 seconds print process ID, consumed CPU time. Every 1.0 second sched_yeild.
* Using sched_setaffinity to lock to processor 0.
*
*
* Example output (aarch64 kernel 5.4)
* [453530] Consumed 0.250001 seconds
* [453530] Consumed 0.500002 seconds
* [453530] Consumed 0.750002 seconds
* [453530] Consumed 1.000006 seconds
* [453531] Consumed 0.250001 seconds
* [453531] Consumed 0.500001 seconds
* [453531] Consumed 0.750005 seconds
* [453531] Consumed 1.000007 seconds
* [453530] Consumed 1.250008 seconds
* [453530] Consumed 1.500009 seconds
* [453530] Consumed 1.750013 seconds
* [453530] Consumed 2.000016 seconds
* [453531] Consumed 1.250007 seconds
* [453531] Consumed 1.500011 seconds
* [453531] Consumed 1.750012 seconds
* [453531] Consumed 2.000014 seconds
* [453530] Consumed 2.250020 seconds
* [453530] Consumed 2.500023 seconds
* [453530] Consumed 2.750028 seconds
* [453530] Consumed 3.000029 seconds
* [453531] Consumed 2.250017 seconds
* [453531] Consumed 2.500017 seconds
* [453531] Consumed 2.750018 seconds
* [453531] Consumed 3.000018 seconds
*/
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void exit_error() {
	printf("rtfifo: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	clock_t rstart, istart, now;
	struct sched_param param;
	int quarters;
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(0, &set);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) == -1)
		exit_error();
	if ((param.sched_priority = sched_get_priority_min(SCHED_FIFO)) == -1)
		exit_error();
	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1)
		exit_error();
	if (fork() == -1) 
		exit_error();
	if ((rstart = clock()) == (clock_t)-1)
		exit_error();
	istart = rstart;
	quarters = 0;
	while (1) {
		if ((now = clock()) == (clock_t)-1)
			exit_error();
		if (now - istart >= (CLOCKS_PER_SEC / 4)) {
			quarters++;
			istart = now;
			printf("[%d] Consumed %f seconds\n", getpid(), (now - rstart) /
				(double)CLOCKS_PER_SEC);
			if (quarters >= 12)
				break;
			if (quarters % 4 == 0)
				sched_yield();
		}
	}
	exit(EXIT_SUCCESS);
}