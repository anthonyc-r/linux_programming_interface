/**
 * An implementation of getpwnam, using setpwent, getpwent, and endpwent.
 */
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

struct passwd *mygetpwdnam(const char *name) {
	struct passwd *pwd;
	while ((pwd = getpwent()) != NULL) {
		if (strcmp(pwd->pw_name, name) == 0) {
			endpwent();
			return pwd;
		}
	}
	endpwent();
	return NULL;
}

int main(int argc, char **argv) {
	struct passwd *pwd;
	
	printf("entry\n");
	errno = 0;
	pwd = mygetpwdnam("root");
	printf("exit\n");
	if (pwd != NULL) {
		printf("found pwd entry! %s\n", pwd->pw_name);
		printf("errno set to %ld\n", errno);
	} else if (errno != 0) {
		printf("null pwd, errno set! to %ld\n", errno);
		exit(EXIT_FAILURE);
	} else {
		printf("pwd entry not found\n");
	}
	exit(EXIT_SUCCESS);
}
