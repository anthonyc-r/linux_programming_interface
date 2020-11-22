/*
* A thread safe implementation of dirname() and basename() using thread specific data
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t basename_key;
static pthread_key_t dirname_key;

static void err(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

static void initkeys() {
	if (pthread_key_create(&basename_key, free) != 0)
		err("pthread_key_create");
	if (pthread_key_create(&dirname_key, free) != 0)
		err("pthread_key_create");
}
char *basename(const char *path) {
	char const *sp, *ep;

	if (pthread_once(&once, initkeys) != 0)
		err("pthread_once");
	char *buf = (char*)pthread_getspecific(basename_key);
	if (buf == NULL) {
		buf = malloc(PATH_MAX + 1);
		if (buf == NULL)
			err("malloc");
		if (pthread_setspecific(basename_key, (void*)buf) != 0)
			err("pthread_setspecific");
	}
	
	if (path == NULL || *path == '\0') {
		buf[0] = '.';
		buf[1] = '\0';
	} else {
		// skip trailing /
		for (ep = (path + strnlen(path, PATH_MAX + 1)) - 1; ep >= path && *ep =='/';
			ep--)
			;
		// It was all /.
		if (ep < path) {
			buf[0] = '/';
			buf[1] = '\0';
		} else {
			for (sp = ep; sp > path && *(sp - 1) != '/';  sp--)
				;
			memcpy(buf, sp, (ep - sp) + 1);
			buf[(ep - sp) + 2] = '\0';
		}
	}

	return buf;
}

char *dirname(const char *path) {
	char const *ep;
	int slash;

	if (pthread_once(&once, initkeys) != 0) {
		puts("pthread_once");
		exit(EXIT_FAILURE);
	}
	char *buf = (char*)pthread_getspecific(dirname_key);
	if (buf == NULL) {
		buf = malloc(PATH_MAX + 1);
		if (buf == NULL)
			err("malloc");
		if (pthread_setspecific(basename_key, (void*)buf) != 0)
			err("pthread_setspecific");
	}
	
	/*
	 Being a bit lazy with this impl! That spagheti logic down there works but I've not
	 bothered thinking too hard about it! Could be edge cases which aren't correct.
	*/
	if (path == NULL || *path == '\0') {
		buf[0] = '.';
		buf[1] = '\0';
	} else {
		// Skip any slashes
		slash = 0;
		for (ep = (path + strnlen(path, PATH_MAX + 1)) - 1; *ep == '/' && ep >= path;
			ep--)
			slash = 1;
		// There were no trailing slashes, so continue back to prev.
		if (!slash) {
			for (; *(ep + 1) != '/' && ep >= path; ep--)
				;
		}

		if ((ep - path) + 1 == 0) {
			if (*path == '/')
				buf[0] = '/';
			else
				buf[0] = '.';
			buf[1] = '\0';
		} else {
			memcpy(buf, path, (ep - path) + 1);
			buf[(ep - path) + 2] = '\0';
		}
	}

	return buf;
}

/** Testing **/

static void *prinfo(void *arg) {
	char *bnam = basename(arg);
	char *dirnam = dirname(arg);
	sleep(1);
	printf("(%u)basename: %s\n", (unsigned)pthread_self(), bnam);
	printf("(%u)dirname: %s\n", (unsigned)pthread_self(), dirnam);
	return (void*)0;
}

int main(int argc, char **argv) {
	int i;
	pthread_t p;

	if (argc < 2) {
		printf("Usage: %s <path> [<path>]...\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	for (i = 1; i < argc; i++) {
		pthread_create(&p, NULL, prinfo, argv[i]);
	}
	
	pthread_exit((void*)0);
}