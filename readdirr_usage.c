/**
An example usage of readdir_r that prints out files in the current working directory.
Interestingly the manpages mark this as deprecated as readdir as it claims
readdir is threadsafe in most implementations and future (post-2008) POSIX 
specifications will require readdir to be thread-safe, and readdir_r as
obsolete. As of revision 2017 this doesn't seem to be the case...
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stddef.h>
#include <dirent.h>

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	char buf[PATH_MAX];
	struct dirent *dirent;
	DIR *dirp;
	int code;
	size_t len;
	
	len = offsetof (struct dirent, d_name) + NAME_MAX + 1;
	if ((dirent = malloc(len)) == NULL)
		error("malloc");
	if (getcwd(buf, PATH_MAX) == NULL)
		error("getcwd");
	dirp = opendir(buf);
	printf("Contents of directory: '%s'\n", buf);
	while ((code = readdir_r(dirp, dirent, &dirent)) == 0) {
		if (dirent == NULL)
			break;
		puts(dirent->d_name);
	}
	if (code != 0)
		error("readdir_r");
	if (closedir(dirp) == -1)
		error("closedir");
	free(dirent);
	exit(EXIT_SUCCESS);
}
