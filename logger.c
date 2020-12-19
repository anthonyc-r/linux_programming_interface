/*
* A simplified implementation of logger(1)
* ./logger [-l level] message
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#define NLEVEL 8
static const char *const LEVELS[NLEVEL] = {
	"emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"
};
static const int LEVEL[NLEVEL] = {
	LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, 	LOG_DEBUG
};

static void exit_usage() {
	puts("./logger [-l emergency|alert|critical|error|warning|notice|info|debug]"\
		" message");
	exit(EXIT_FAILURE);
}

static void exit_error() {
	printf("logger: %s\n", strerror(errno));
	exit(errno);
}

static char *catargv(int argc, int argoff, char **argv) {
	char *msg;
	int i, cursor, mlen;

	for (i = argoff, mlen = 0; i < argc; i++)
		mlen += strlen(argv[i]) + 1;
	msg = malloc(mlen == 0 ? 1 : mlen);
	msg[0] = '\0';
	for (i = optind, cursor = 0; i < argc; i++) {
		mlen = strlen(argv[i]);
		cursor += mlen + 1;
		strncat(msg, argv[i], mlen);
		msg[cursor - 1] = ' ';
		msg[cursor] = '\0';
	}
	return msg;
}	

int main(int argc, char **argv) {
	int ch, level, i;
	char *levels, *msg;
	
	levels = NULL;
	while ((ch = getopt(argc, argv, "l:")) != -1) {
		switch (ch) {
		case 'l':
			levels = optarg;
			break;
		case '?':
			exit_usage();
		default:
			break;
		}
	}
	if (levels != NULL && argc < 3)
		exit_usage();
	else if (argc < 2)
		exit_usage();
	
	if ((msg = catargv(argc, optind, argv)) == NULL)
		exit_error();

	if (levels != NULL) {
		for (i = 0; i < NLEVEL; i++) {
			if (strncmp(LEVELS[i], levels, strlen(levels) + 1) == 0) {
				level = LEVEL[i];
				break;
			}
		}
		if (i == NLEVEL)
			exit_usage();
	} else {
		level = LOG_INFO;
	}
	syslog(LOG_LOCAL0|level, "%s", msg);
	free(msg);
	exit(EXIT_SUCCESS);
}