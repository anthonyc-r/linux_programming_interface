/*
* Receiving a message via a posix message queue - making use of the 
* 'timed' variant.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <mqueue.h>
#include <signal.h>

#define MQ_NAME "/pmq_rcv"
#define MLEN 100
#define TTIME 5

static mqd_t q;

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void sendmsg(int sig) {
	if (mq_send(q, "hello!", 7, 1) == -1)
		exit_err("mq_send");
}

int main(int argc, char **argv) {
	struct timespec ts;
	char msg[MLEN];
	struct mq_attr attr;
	sigset_t block;
	struct sigaction sigact;
	pid_t child;

	// Create mq if not exists.
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MLEN;
	if ((q = mq_open(MQ_NAME, O_CREAT |  O_RDWR, S_IRWXU, &attr)) == (mqd_t)-1)
		exit_err("mq_open");

	// Set up signals and child proc to handle sending a message on sigint
	switch (child = fork()) {
	case -1:
		exit_err("fork");
	case 0:
		sigact.sa_handler = sendmsg;
		sigact.sa_flags = 0;
		sigemptyset(&sigact.sa_mask);
		if (sigaction(SIGINT, &sigact, NULL) == -1)
			exit_err("sigaction");
		pause();
		_exit(EXIT_SUCCESS);
	default:
		break;
	}
	sigemptyset(&block);
	sigaddset(&block, SIGINT);
	if (sigprocmask(SIG_BLOCK, &block, NULL) == -1)
		exit_err("sigprocmask");

	// timed receive
	ts.tv_sec = time(NULL) + TTIME;
	printf("mq_timedreceive with timeout of %d seconds\n", TTIME);
	printf("press ctrl+c to send message on queue\n");
	if (mq_timedreceive(q, msg, MLEN, NULL, &ts) == -1)
		exit_err("mq_timedreceive");
	else
		printf("Message received: %.*s\n", MLEN, msg);
	kill(child, SIGQUIT);
	exit(EXIT_SUCCESS);
}
