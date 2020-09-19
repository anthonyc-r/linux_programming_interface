/*
An implementation of abort()
The abort function must guarentee that the program terminates and produces a core dump file, unless a 
custom SIGABRT handler has been set which does not return (e.g. longjmps...)
*/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

void my_abort() {
	struct sigaction sigact;
	
	// Raise a SIGABRT
	raise(SIGABRT);
	// Program hasn't terminated. Set disposition of SIGABT to default
	memset(&sigact, 0, sizeof (struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sigact, NULL);
	// Raise SIGABRT again!
	raise(SIGABRT);
}


// Test the above abort!
static void handler(int sig) {
	// Should be safe, as both of these functions are async-signal-safe as of SUSv3.
	// As opposed to using STDIO functions, which due to stdio buffering are not!
	write(STDOUT_FILENO, "handler called!\n", 16);
	fsync(STDOUT_FILENO);	
}

int main(int argc, char **argv) {
	struct sigaction sigact;
	
	// Set disposition of SIGABRT to something which does not terminate the program and returns
	memset(&sigact, 0, sizeof (struct sigaction));
	sigact.sa_handler = handler;
	sigaction(SIGABRT, &sigact, NULL);
	
	puts("Calling my_abort!");
	my_abort();
	puts("This shouldn't be reached!");

	exit(EXIT_SUCCESS);	
}
