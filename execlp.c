/*
* An implementation of execlp, using execve
*/
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

extern char **environ;

int execlp(const char *file, const char *arg, ...) {
	va_list ap;
	char const **argv, *path;
	char *paths, fpath[PATH_MAX + 1], *last;
	int argc, ret;

	if (file == NULL) {
		errno = EFAULT;
		return -1;
	}

	va_start(ap, arg);
	for (argc = 0; va_arg(ap, const char*) != NULL; argc++)
		;
	if ((argv = malloc((argc + 2) * sizeof (const char*))) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	argv[0] = arg;
	va_start(ap, arg);
	for (argc = 1; (argv[argc] = va_arg(ap, const char*)); argc++)
		;

	if (file[0] == '/') {
		ret = execve(path, (char *const *)argv, environ);
	} else {
		paths = getenv("PATH");
		if (paths == NULL)
			// OpenBSD actually specifies that this gets set to
			// _PATH_DEFPATH - which includes sbin and X11R6/bin also.
			paths = ".:/usr/bin:/bin";
		for (path = strtok_r(paths, ":", &last); path; 
			path = strtok_r(NULL, ":", &last)) {
			snprintf(fpath, PATH_MAX, "%s/%s", path, file);
			fpath[PATH_MAX] = '\0';
			printf("checking path at %s\n", fpath);
			ret = execve(fpath, (char *const *)argv, environ);
			printf("errno: %d\n", errno);
		}
	}
	free(argv);
	return ret;
}

int main(int argc, char **argv) {
	execlp("echo", "echo", "hello!", (const char*)NULL);
	exit(EXIT_FAILURE);
}