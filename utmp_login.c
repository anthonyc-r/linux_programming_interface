/*
* This program simulates a user logging in for some number of seconds, and then logging out
* ensuring utmp, wtmp and lastlogin are set correctly.
* Tested on OpenBSD.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>

static struct utmp utmp;
static int utmp_fd, wtmp_fd, lastlog_fd;
static struct lastlog lastlog;

static void exit_error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}
static void exit_errno(char *reason) {
	printf("utmp_login(%s) - %s\n", reason, strerror(errno));
	exit(errno);
}


static void open_files() {
	if ((wtmp_fd = open(_PATH_WTMP, O_WRONLY | O_APPEND)) == -1)
		exit_errno("open wtmp");
	if ((utmp_fd = open(_PATH_UTMP, O_RDWR)) == -1)
		exit_errno("open utmp");
	if ((lastlog_fd = open(_PATH_LASTLOG, O_RDWR)) == -1)
		exit_errno("open lastlog");
}

static void close_files() {
	close(wtmp_fd);
	close(utmp_fd);
	close(lastlog_fd);
}

static void seek_utmp(const char *const unam) {
	off_t off;
	struct utmp buf;
	
	if ((off = lseek(utmp_fd, 0, SEEK_SET)) == -1)
		exit_errno("lseek");
	while (read(utmp_fd, &buf, sizeof (utmp)) == sizeof (utmp)) {
		if (strncmp(buf.ut_name, utmp.ut_name, UT_NAMESIZE) == 0)
			break;		
		if ((off = lseek(utmp_fd, 0, SEEK_CUR)) == -1)
			exit_errno("lseek");
	}
	if ((off = lseek(utmp_fd, off, SEEK_SET)) == -1)
		exit_errno("lseek");
}

static void write_tmp(int tmp_fd) {
	if (write(tmp_fd, &utmp, sizeof (struct utmp)) != sizeof (struct utmp))
		exit_errno("write tmp");
}

static void write_lastlog(uid_t uid) {
	if (lseek(lastlog_fd, uid * sizeof (struct lastlog), SEEK_SET) == -1)
		exit_errno("lseek");
	if (write(lastlog_fd, &lastlog, sizeof (struct lastlog)) != sizeof (struct lastlog))
		exit_errno("write lastlog");
}

int main(int argc, char **argv) {
	char *ttyn, *line;
	int fd;
	struct passwd *pwd;
	struct utmp buf;

	if (argc < 2)
		exit_error("Usage: ./tmp_login name");
	if ((pwd = getpwnam(argv[1])) == NULL)
		exit_errno("getpwnam");

	ttyn = ttyname(STDIN_FILENO);
	if (ttyn == NULL)
		exit_errno("ttyname");
	line = basename(ttyn);
	strncpy((char*)&utmp.ut_line, line, UT_LINESIZE);
	strncpy((char*)&utmp.ut_name, argv[1], UT_NAMESIZE);
	time(&utmp.ut_time);
	
	lastlog.ll_time = utmp.ut_time;
	strncpy((char*)&lastlog.ll_line, line, UT_LINESIZE);

	open_files();
	seek_utmp(argv[1]);
	write_tmp(utmp_fd);
	write_tmp(wtmp_fd);
	write_lastlog(pwd->pw_uid);
	
	puts("User logged in. Sleeping for 5...");
	sleep(5);

	puts("Logging user out.");
	seek_utmp(argv[1]);
	time(&utmp.ut_time);
	memset(&utmp.ut_name, 0, UT_NAMESIZE);
	write_tmp(utmp_fd);
	write_tmp(wtmp_fd);

	close_files();
	exit(EXIT_SUCCESS);
}