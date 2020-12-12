/**
* Run with exec to ensure this becomes the controlling process.
* Check that if the controlling process catches sighup then the kernel wont send
* it to all processes in the foreground process group.
*
* To demonstrate this, start the program with exec ./progname, then after close the terminal
* window. Then check the output of sighup_controllingp_output.txt, to see that only the
* parent got a sighup.
*
* Commenting out line 44, and repeating the above, you will instead see that all the
* children receieve the SIGHUP!
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>


void handler(int sig) {
	printf("[%d]SIGHUP\n", getpid());
}

int main(int argc, char **argv) {	
	close(STDOUT_FILENO);
	open("sighup_controllingp_output.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR); 
	setbuf(stdout, NULL);
	printf("[%d]PARENT\n", getpid());
	alarm(10);
	for (int i = 0; i < 10; i++) {
		switch(fork()) {
		case -1:
			puts("fork");
			exit(EXIT_FAILURE);
		case 0:
			signal(SIGHUP, handler);
			while (1) {
				pause();
			}
			_exit(EXIT_SUCCESS);
			break;
		default:
			signal(SIGHUP, handler);
			break;
		}
	}
	while (1) {
		pause();
	}
	exit(EXIT_SUCCESS);
}
