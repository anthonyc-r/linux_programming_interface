/**
* Linux-specific
* Run a program with a specific realtime scheduler and priority
* Executable should be made set-user-id root.
* $./rtsched policy(r|f) priority command args...
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <string.h>


static void exit_usage() {
	setuid(getuid());
	puts("Usage: ./rtsched policy priority command arg");
	exit(EXIT_FAILURE);
}

static void exit_error() {
	setuid(getuid());
	printf("rtsched: %s\n", strerror(errno));
	exit(errno);
}

int main(int argc, char **argv) {
	int policy;
	struct sched_param param;
	char *ep;
	
	// Temporarily drop priv
	if (seteuid(getuid()))
		exit_error();
	if (argc < 4)
		exit_usage();
	param.sched_priority = strtol(argv[2], &ep, 10);
	if (*ep != '\0')
		exit_usage();
	if (*argv[1] == 'f')
		policy = SCHED_FIFO;
	else if (*argv[1] == 'r')
		policy = SCHED_RR;
	else
		exit_usage();
	// Restore priv
	if (seteuid(0))
		exit_error();
	if (sched_setscheduler(0, policy, &param) == -1)
		exit_error();
	// Permenantly drop priv
	if (setuid(getuid()))
		exit_error();
	execvp(argv[3], argv + 3);
	exit_error();
}