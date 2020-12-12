#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv) {
	pid_t pid, sid;

	switch (pid = fork()) {
	case 0:
		if ((sid = setsid()) == -1) {
			puts("initial sid unexpectedly failed");
			exit(EXIT_FAILURE);
		}
		// We're now the session leader...
		if ((sid = setsid()) == -1) {
			if (errno == EPERM)
				puts("setsid() failed with EPERM!");
			else
				puts("setsid() failed with unexpected errno");
		}
		
		break;
	case -1:
		exit(EXIT_FAILURE);
		break;
	default:
		break;
	}
	
	exit(EXIT_SUCCESS);
}