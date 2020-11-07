/**
 Examine what happens when a thread tries to join with itself. Does it halt indefinitely?
 Example on OpenBSD:
netty$ ./pthread_join_self
pthread_join
Exiting program, thread joined with return: 1
 We can see that calling pthread_join providing pthread_self results in an error.
 To prevent this, it may be prudent to wrap pthread_join in an equality check.

 ------
 Note that SUSv3 also permits that the thread deadlocks! So when writing portable code
 and there's a possibility of passing it's own thread to pthread_join, always wrap it in a 
 check. 
 Remember that (tid != pthread_self()) is NOT okay, you should use the pthread_equals 
 function! 
*/

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void *thread_fun(void *arg) {
	if (pthread_join(pthread_self(), NULL) != 0) {
		puts("pthread_join");
		pthread_exit((void*)1);
	}
	return (void*)0;
}

int main(int argc, char **argv) {
	pthread_t thread;
	void *retval;

	if (pthread_create(&thread, NULL, thread_fun, NULL) != 0) {
		puts("pthread_create");
		exit(EXIT_FAILURE);
	}
	pthread_join(thread, &retval);
	// Note the possible collision with PTHREAD_CANCELLED when casting ret
	// to int.
	printf("Exiting program, thread joined with return: %d\n", (int)retval);
	exit(EXIT_SUCCESS);
}