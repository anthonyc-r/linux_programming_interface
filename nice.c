/*
* An implementation of the nice(1) command
* From the man page:
* nice [-n increment] utility [argument ...]
* nice runs utility at an altered scheduling priority.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void exit_usage() {
	puts("nice [-n increment] utility [argument ...]");
	exit(EXIT_FAILURE);
}

static void exit_error() {
	printf("nice: %s\n", strerror(errno));
	exit(errno);
}	

int main(int argc, char **argv) {
	int ch, incr, incr_set;
	char *ep, *util;

	if (argc < 2)
		exit_usage();
	incr_set = 0;
	while ((ch = getopt(argc, argv, "n:")) != -1) {
		switch (ch) {
		case 'n':
			incr = (int)strtol(optarg, &ep, 10);
			if (*ep != '\0')
				exit_usage();
			incr_set = 1;
			break;	
		case '?':
			exit_usage();
		default:
			break;
		}
	}
	if (incr_set && argc < 3)
		exit_usage();
	else if (!incr_set)
		incr = 10;
	util = argv[optind];
	errno = 0;
	if (nice(incr) == -1 && errno)
		exit_error();
	execvp(util, argv + optind);
	exit_error();
}