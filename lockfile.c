/**
 * A simple version of the lockfile utility that comes with the 
 * procmail package.
 * lockfile -p sleeptime | -r retries | -l locktimeout | 
 * 	-s suspend | -! | filename...
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define MAXFILES 100
#define NO_LOCKTIMEOUT -1

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static long getlong(char *str, char *argnam) {
	char *ep;
	long val;

	val = strtol(str, &ep, 10);
	if (ep == NULL || *ep != '\0') {
		printf("Invalid arg: %s\n", argnam);
		exit(EXIT_FAILURE);
	}
	return val;
}

static int timed_out(struct timespec *ts, int locktimeout) {
	time_t now;

	if (locktimeout == NO_LOCKTIMEOUT)
		return 0;
	if ((now = time(NULL)) == (time_t) -1)
		exit_err("time");
	if ((now - ts->tv_sec) >= locktimeout)
		return 1;
	else
		return 0;
}


int main(int argc, char **argv) {
	long sleeptime, retries, locktimeout, suspend;
	int invert, opt, fdv[MAXFILES], i, nretries;
	struct stat sb;
	
	sleeptime = 8;
	retries = -1;
	locktimeout = NO_LOCKTIMEOUT;
	suspend = 16;
	invert = 0;
	while ((opt = getopt(argc, argv, "r:l:s:p:!")) != -1) {
		switch (opt) {
		case 'r':
			retries = getlong(optarg, "retries");
			break;
		case 'l':
			locktimeout = getlong(optarg, "locktimeout");
			break;
		case 's':
			suspend = getlong(optarg, "suspend");
			break;
		case 'p':
			sleeptime = getlong(optarg, "sleeptime");
			break;
		case '!':
			invert = 1;
			break;
		case '?':
			exit(EXIT_FAILURE);
		default:
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc > MAXFILES)
		exit_err("Too many files");
	nretries = 0;
	for (i = 0; i < argc; i++) {
		if (stat(argv[i], &sb) != -1 && timed_out(&sb.st_mtim, locktimeout)) {
			if (unlink(argv[i]) == -1)
				exit_err("unlink timed out lockfile");
			sleep(suspend);
		}
		fdv[i] = open(argv[i], O_CREAT | O_EXCL, S_IRUSR | S_IROTH | S_IRGRP);
		if (fdv[i] == -1) {
			if (errno != EEXIST)
				exit_err("open");
			if (retries == -1 || nretries < retries) {
				nretries++;
				i--;
				sleep(sleeptime);
			} else {
				if (invert)
					exit(EXIT_SUCCESS);
				else
					exit(EXIT_FAILURE);
			}
		}
	}
	if (invert)
		exit(EXIT_FAILURE);
	else
		exit(EXIT_SUCCESS);
}
