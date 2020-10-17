/*
* Verify if a child process gets adopted by init when it's parent becomes
* a zombie, or only after the parent is waited on.
* Would be better to use signals to sync this up, but for the sake of 
* convenience, sleep() is used!
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

static void grandparent() {
	int status;
	// T0: Parent killed
	// T1: Parent is zombie
	// T2: Parent is reaped here...
	sleep(2);
	wait(&status);
	printf("Grandparent reaped zombie parent\n");
	// Hang around a bit to keep the process in the forground...
	sleep(2);
}

static void parent() {
	// Dies immediately to become a zombie.
	printf("Parent exiting...\n");
}

static void child() {
	sleep(1);
	printf("Parent is a zombie and my ppid is: %d\n", getppid());
	sleep(2);
	// T3: Check ppid...
	printf("Parent should have been reaped now, my ppid is %d\n", getppid());
}

static void err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	switch (fork()) {
		case -1: 
			err("fork1");
		case 0:
			// Parent
			switch (fork()) {
				case -1:
					err("fork2");
				case 0:
					child();
					break;
				default:
					parent(); 
			}
			break;
		default:
			grandparent();
			
	}
	exit(EXIT_SUCCESS);
}
