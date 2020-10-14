/*
* This program demonstrates how after a vfork(), although the program data is shared,
* the file descriptor table is duplicated, so we can safely close a file descriptor 
* without affecting the parent!
*/
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int status;
	
	puts("Spawning a child and closing file descriptor 0 (stdout)");
	switch (vfork()) {
	case -1:
		puts("vfork");
		exit(EXIT_FAILURE);
	case 0:
		write(0, "(child) Closing stdout!\n", 25);
		close(0);
		_exit(0);
	default:
		wait(&status);
		write(0, "(parent) Hey we can still write out on 0!\n", 43);
	}
	
	exit(EXIT_SUCCESS);	
}
