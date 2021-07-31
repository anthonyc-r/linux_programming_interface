/**
* A simple local chat program making use of posix message queues
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define MLEN 200
#define QPERM S_IRWXU | S_IRWXO

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

void mreceived(int sig, siginfo_t *info, void *data) {
	/*
	Can't actually see any way of getting mqd from info as the book
	suggests... Only way I can think of getting it without already
	knowing it is by mounting the message queue filesystem and
	checking that.
	*/
	printf("Siginfo:\n");
	printf("si_signo: %d\n", info->si_signo);
	printf("si_pid: %d\n", info->si_pid);
	printf("si_uid: %d\n", info->si_uid);
	printf("si_code: %d\n", info->si_code);
	printf("sival_ptr: %p\n", info->si_value.sival_ptr);
}

int main(int argc, char **argv) {
	mqd_t q, qrcv, qsnd;
	char qnam[PATH_MAX], msg[MLEN];
	struct mq_attr mattr;
	struct sigevent sev;
	struct sigaction sigact;

	if (argc < 2)
		exit_err("usage: ./chat <queue identifier>");
	snprintf(qnam, PATH_MAX, "%s_aux", argv[1]);
	mattr.mq_maxmsg = 10;
	mattr.mq_msgsize = MLEN;
	q = mq_open(argv[1], O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK, QPERM, &mattr);
	if (q == (mqd_t)-1 && errno != EEXIST) {
		exit_err("mq_open 1");
	} else if (q == (mqd_t)-1) {
		if ((qsnd = mq_open(argv[1], O_RDWR)) == (mqd_t)-1)
			exit_err("mq_open2");
		if ((qrcv = mq_open(qnam, O_RDWR | O_CREAT | O_NONBLOCK, QPERM, &mattr)) == (mqd_t)-1)
			exit_err("mq_open3");
	} else {
		qrcv = q;
		if ((qsnd = mq_open(qnam, O_RDWR | O_CREAT | O_NONBLOCK, QPERM, &mattr)) == (mqd_t)-1)
			exit_err("mq_open4");
	}

	printf("Receiving mq descriptor: %d\n", qrcv);

	sigemptyset(&sigact.sa_mask);
	sigact.sa_sigaction = mreceived;
	sigact.sa_flags = SA_SIGINFO;
	if (sigaction(SIGUSR1, &sigact, NULL) == -1)
		exit_err("sigaction");

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGUSR1;
	while (1) {
		if (mq_notify(qrcv, &sev) == -1 && errno != EBUSY)
			exit_err("mq_notify");
		while (mq_receive(qrcv, msg, MLEN, NULL) != -1) {
			printf("(them): %.*s\n", MLEN, msg);
		}
		if (fgets(msg, MLEN, stdin) != NULL) {
			if (mq_send(qsnd, msg, MLEN, 1) == -1)
				exit_err("mq_send msg");
			printf("\033[1A(you): %.*s\n", MLEN, msg);
		}
	}
	exit(EXIT_SUCCESS);
}
