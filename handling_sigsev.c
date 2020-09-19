/*
A silly example of catching a SIGSEGV and handling what went wrong. 
*/
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>


static jmp_buf env;

static void handler(int sig) {
	longjmp(env, 1);
}

int main(int argc, char *argv) {
	char *string;
	struct sigaction sigact, old_sigact;
	
	memset(&sigact, 0, sizeof (struct sigaction));
	sigact.sa_handler = handler;
	sigaction(SIGSEGV, &sigact, &old_sigact);
		
	string = NULL;
	
	switch (setjmp(env)) {
		case 0: // Normal execution
			break;
		default: // jumped back!
			puts("Oops! A SIGSEGV was raised! Fix the probable cause...");
			string = malloc(5);
			strncpy(string, "_ony", 5);		
	}
	
	puts("Attempt to acccess string...");
	string[0] = 't';
	printf("Hello %s!\n", string);
	// Should definitely set this back to default after the suspect code!
	sigaction(SIGSEGV, &old_sigact, NULL);
	
	exit(EXIT_SUCCESS);
}