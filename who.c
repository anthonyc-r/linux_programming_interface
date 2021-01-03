/*
* Implementation of who(1)
* Based off of the OpenBSD manpage. 
* Tested on OpenBSD.
*/

#include <pwd.h>
#include <errno.h>
#include <time.h>
#include <utmp.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#define _TIMELEN 50

extern int optind;
static 	int ch, header, me, quick, tstate, idle;

static void exit_usage() {
	puts("who [-HmqTu] [file]");
	exit(EXIT_FAILURE);
}

static void exit_errno(char *reason) {
	printf("who(%s) - %s\n", reason, strerror(errno));
	exit(errno);
}	

int main(int argc, char **argv) {
	int fd, tfd, count;
	size_t nread, ntime;
	char *path, timestr[_TIMELEN], state, device[PATH_MAX];
	struct utmp utmp;
	struct stat statb;
	struct timespec now;
	time_t dtime;
	
	while ((ch = getopt(argc, argv, "HmqTu")) != -1) {
		switch (ch) {
		case 'H':
			header = 1;
			break;
		case 'm':
			me = 1;
			break;
		case 'q':
			quick = 1;
			break;
		case 'T':
			tstate = 1;
			break;
		case 'u':
			idle = 1;
			break;
		default:
			exit_usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 0 && strncmp(argv[0], "am", 3))
		me = 1;
	else if (argc > 0)
		path = argv[0];
	else 
		path = _PATH_UTMP;

	if ((fd = open(path, O_RDONLY)) == -1)
		exit_errno("open");
	if (quick) {
		count = 0;
		while ((nread = read(fd, &utmp, sizeof (utmp))) == sizeof (utmp)) {
			if (strlen(utmp.ut_name) != 0) {
				printf("%s\t", utmp.ut_name);
				count++;
			}
		}
		if (nread == -1)
			exit_errno("read");
		puts("");
		printf("# users=%d\n", count);
	} else {
		if (header) {
			printf("USER\t");
			if (tstate)
				printf("S\t");
			printf("LINE\tWHEN\t");
			if (idle)
				printf("IDLE\t");
			printf("FROM\n");
		}
		while ((nread = read(fd, &utmp, sizeof (utmp))) == sizeof (utmp)) {
			if (strlen(utmp.ut_name) == 0 || (me && strncmp(utmp.ut_name, 
				getlogin(), UT_NAMESIZE) != 0)) {
				continue;
			}
			ntime = strftime(timestr, _TIMELEN, "%b %e %H:%M",
				localtime(&utmp.ut_time));
			if (ntime == -1)
				exit_errno("strftime");
			printf("%s\t", utmp.ut_name);
			snprintf(device, PATH_MAX, "/dev/%s", utmp.ut_line);
			if (tstate) {
				if ((tfd = open(device, O_WRONLY)) == -1) {
					if (errno == EACCES)
						state = '-';
					else
						state = '?';
				} else {
					state = '+';
					close(tfd);
				}
				printf("%c\t", state);
			}
			printf("%s\t%s\t", utmp.ut_line, timestr);
			if (idle) {
				if (clock_gettime(CLOCK_REALTIME, &now) == -1)
					exit_errno("clock_gettime");
				if (stat(device, &statb) == -1)
					exit_errno("stat");
				dtime = (now.tv_sec - statb.st_mtim.tv_sec) / 60;
				if (dtime < 1)
					printf("%s\t", ".");
				else
					printf("%02lld:%02lld\t", dtime / 60, dtime % 60);
			}
			printf("%s\n", utmp.ut_host);
		}
		if (nread == -1)
			exit_errno("read");
	}
	if (close(fd) == -1)
		exit_errno("close");
	exit(EXIT_SUCCESS);
}