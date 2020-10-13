/* 
* This program demonstrates the equivalence using POSIX timers of specifying NULL for evp
* with timer_create(), and specifying sigev_notify <- SIGEV_SIGNAL, sigev_signo <- SIGALRM
* si_value.sival_int <- timer ID
*/
#define TIMEOUT 5


#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

static char *getime() {
	static char string[100];
	struct tm *tm;
	time_t time;
	ctime(&time);
	tm = localtime(&time);
	strftime(string, 100, "%T", tm);
	return string;
}

static void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	struct itimerspec timespec;
	struct sigevent sigev;
	sigset_t block;
	siginfo_t signfo;
	timer_t timer;

	printf("[%s] Creating timer with NULL evp!\n", getime());
	memset(&timespec, 0, sizeof(struct itimerspec));
	timespec.it_value.tv_sec = TIMEOUT;
	sigemptyset(&block);
	sigaddset(&block, SIGALRM);
	sigprocmask(SIG_BLOCK, &block, NULL);
	if (timer_create(CLOCK_REALTIME, NULL, &timer) == -1)
		error("timer_create");
	if (timer_settime(timer, 0, &timespec, NULL) == -1)
		error("timer_settime");
	if (sigwaitinfo(&block, &signfo) == -1)
		error("sigwaitinfo");
	printf("[%s] SIGALRM! Value: %d\n", getime(),
		signfo.si_value.sival_int);
	if (timer_delete(timer) == -1)
		error("timer_delete");

	printf("[%s] Creating timer with evp SIGEV_SIGNAL, SIGALRM"
		" sigval = timerid\n", getime());
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = SIGALRM;
	sigev.sigev_value.sival_int = (long)timer;
	if (timer_create(CLOCK_REALTIME, &sigev, &timer) == -1)
		error("timer_create");
	if (timer_settime(timer, 0, &timespec, NULL) == -1)
		error("timer_settime");
	if (sigwaitinfo(&block, &signfo) == -1)
		error("sigwaitinfo");
	printf("[%s] SIGALRM! Value: %d\n", getime(),
		signfo.si_value.sival_int);
	if (timer_delete(timer) == -1)
		error("timer_delete");
	

	exit(EXIT_SUCCESS);
}
