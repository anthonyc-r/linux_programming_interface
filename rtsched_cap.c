/**
* Linux-specific
* Run a program with a specific realtime scheduler and priority
* This version still assumes the file is set-uid-root, and doesn't assume use of
* file capabilities, however it does make use of capabilities via libcap rather than
* changing the UID.
* $./rtsched policy(r|f) priority command args...
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <linux/securebits.h>
#include <linux/capability.h>


static void exit_usage() {
	setuid(getuid());
	puts("Usage: ./rtsched policy priority command arg");
	exit(EXIT_FAILURE);
}

static void exit_error(char *tag) {
	setuid(getuid());
	printf("rtsched (%s): %s\n", tag, strerror(errno));
	exit(errno);
}



// Want to drop root permenantly, but retain required caps.
static void drop_root() {
	cap_t caps;
	cap_value_t cap;

	if (prctl(PR_SET_SECUREBITS, SECBIT_NO_SETUID_FIXUP | SECBIT_NOROOT |
		SECBIT_NO_SETUID_FIXUP_LOCKED | SECBIT_NOROOT_LOCKED) == -1)
		exit_error("prctl");
	if ((caps = cap_init()) == NULL)
		exit_error("cap_init");

	cap = CAP_SYS_NICE;
	if (cap_set_flag(caps, CAP_PERMITTED, 1, &cap, 1) == -1)	
		exit_error("cap_set_flag");
	if (cap_set_proc(caps) == -1)
		exit_error("cap_set_proc");
	if (cap_free(caps) == -1)
		exit_error("cap_free");
	if (setuid(getuid()) == -1)
		exit_error("setuid");
}

static void drop_caps(int forever) {
	cap_flag_t flags;
	cap_t caps;
	cap_value_t cap;

	if ((caps = cap_init()) == NULL)
		exit_error("cap_init");
	cap = CAP_SYS_NICE;
	if (!forever) {
		if (cap_set_flag(caps, CAP_PERMITTED, 1, &cap, 1) == -1)	
			exit_error("cap_set_flag");
	}
	if (cap_set_proc(caps) == -1)
		exit_error("cap_set_proc");
	if (cap_free(caps) == -1)
		exit_error("cap_free");
}

static void raise_caps() {
	cap_t caps;
	cap_value_t cap;

	if ((caps = cap_init()) == NULL)
		exit_error("cap_init");
	cap = CAP_SYS_NICE;
	if (cap_set_flag(caps, CAP_PERMITTED, 1, &cap, 1) == -1)	
		exit_error("cap_set_flag");
	if (cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, 1) == -1)	
		exit_error("cap_set_flag");
	if (cap_set_proc(caps) == -1)
		exit_error("cap_set_proc");
	if (cap_free(caps) == -1)
		exit_error("cap_free");
}

int main(int argc, char **argv) {
	int policy;
	struct sched_param param;
	char *ep;
	
	drop_root();
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

	raise_caps();
	if (sched_setscheduler(0, policy, &param) == -1)
		exit_error("sched_setscheduler");
	drop_caps(1);
	if (setuid(getuid()))
		exit_error("setuid");
	execvp(argv[3], argv + 3);
	exit_error("execve");
}