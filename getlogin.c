/**
* An implementation of getlogin(2) using the utmp file and ttyname
* Tested on OpenBSD which doesn't appear to have the utmpx functions, so the file is 
* opened and read manually
*/
#include <time.h>
#include <fcntl.h>
#include <utmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static void exit_error(char *reason) {
	printf("./getlogin (%s) - %s\n", reason, strerror(errno));
}	

char *getlogin2(void) {
	static char login[UT_NAMESIZE];
	char *ttyn;
	int fd;
	struct utmp utmp;

	if ((ttyn = ttyname(STDOUT_FILENO)) == NULL)
		exit_error("ttyname");
	if ((fd = open(_PATH_UTMP, O_RDONLY)) == -1)
		exit_error("open utmp");
	while (read(fd, &utmp, sizeof(struct utmp)) == sizeof(struct utmp)) {
		if (strncmp(ttyn + 5, utmp.ut_line, UT_LINESIZE) == 0) {
			strncpy(login, utmp.ut_name, UT_NAMESIZE);
			return login;
		}
	}
	return NULL;
}

int main(int argc, char **argv) {
	char *login;

	login = getlogin2();
	if (login != NULL)
		puts(login);
	else
		puts("not found");
	exit(EXIT_SUCCESS);
}