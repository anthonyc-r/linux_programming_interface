/*
* This program verifies that orphaned processes are adopted by the init
* process.
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	switch (fork()) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			// child continues...
			printf("Child forked, it's ppid is: %d\n", getppid());
			sleep(1);
			break;
		default:
			// parent will exit
			puts("Parent is now exiting!");
			exit(EXIT_SUCCESS);
	}
	// Childs ppid should now be 1!
	printf("The child's parent process is now %d\n", getppid());
	exit(EXIT_SUCCESS);
}
