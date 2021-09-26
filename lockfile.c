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
	if (locktimeout == NO_LOCKTIMEOUT)
		return 0;
	return 1;
}


int main(int argc, char **argv) {
	long sleeptime, retries, locktimeout, suspend;
	int invert, opt, fdv[MAXFILES], i;
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

	printf("retries: %ld, locktimeout: %ld, suspend: %ld, sleeptime: %ld\n",
		retries, locktimeout, suspend, sleeptime);
	printf("invert?: %d\n", invert);

	if (argc > MAXFILES)
		exit_err("Too many files");
	for (i = 0; i < argc; i++) {
		if (stat(argv[i], &sb) != -1 && timed_out(&sb.st_mtim, locktimeout)) {
			if (unlink(argv[i]) == -1)
				exit_err("unlink timed out lockfile");
		}
		fdv[i] = open(argv[i], O_CREAT | O_EXCL, S_IRUSR | S_IROTH | S_IRGRP);

	}

	exit(EXIT_SUCCESS);
}
