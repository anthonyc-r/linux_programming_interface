/*
* Example use of waitid()
* This program creates a child, the child then exits with the status code
* provided as an argument, or sleeps indefinitely until it gets
* terminated by a signal.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static void pinfo(siginfo_t *info) {
	char *code;
	switch (info->si_code) {
		case CLD_EXITED:
			code = "CLD_EXITED";
			break;
		case CLD_KILLED:
			code = "CLD_KILLED";
			break;
		case CLD_STOPPED:
			code = "CLD_STOPPED";
			break;
		case CLD_CONTINUED:
			code = "CLD_CONTINUED";
			break;
		default:
			code = "UNKNOWN";
	}
	printf("SIGINFO:\n");
	printf("\tCode: %s\n", code);
	printf("\tPID: %d\n", info->si_pid);
	if (info->si_code == CLD_EXITED) {
		printf("\tEXIT STATUS: %d\n", info->si_status);
	} else {
		printf("\tSIGNAL: %d(%s)\n", info->si_status,
			strsignal(info->si_status));
	}
	printf("\tUID: %d\n", info->si_uid);
}

int main(int argc, char **argv) {
	int estat;
	siginfo_t info;
	
	estat = -1;
	if (argc > 1) {
		estat = (int)strtol(argv[1], NULL, 10);
	}
	switch (fork()) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			// Child
			if (estat > -1) {
				_exit(estat);
			} else {
				while (1) {
					sleep(1);
				}
			}
			// Never reached.
			_exit(EXIT_SUCCESS);
		default:
			break;
	}
	// Parent continues to waitid on child...
	errno = 0;
	while (waitid(P_ALL, 0, &info, WEXITED|WSTOPPED|WCONTINUED) != -1) {
		printf("Child exited with info...\n");
		pinfo(&info);
	}
	if (errno != ECHILD) {
		puts("waitid");
		exit(EXIT_FAILURE);
	}	
	puts("No more children, exiting!");	
	exit(EXIT_SUCCESS);
}
