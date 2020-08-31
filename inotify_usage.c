/**
This program makes use of inotify to monitor a directory and it's subdirectories.
Example usage: inotify_usage <dir>
The program prints out a message whenever any file in <dir> or it's subdirectories has been
modied. If a directory is added or deleted under the tree, the set of watched directories is updated.
*/
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <ftw.h>
#include <sys/inotify.h>
#include <limits.h>
#include <time.h>
#include <string.h>

// struct + variable name length + null delimiter. 
// (There actually may be a variable number of null padding too)
#define BUFSZ (10 * (sizeof (struct inotify_event) + NAME_MAX + 1))
#define ARRAY_BLOCK_SZ 2

struct watch_paths {
	char **paths;
	int size;
	int max_sz;
};

static int inotify_fd;
static struct watch_paths watch_paths;

static void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

static void watch_paths_init() {
	watch_paths.paths = calloc(ARRAY_BLOCK_SZ, sizeof (char *));
	watch_paths.size = 0;
	watch_paths.max_sz = ARRAY_BLOCK_SZ;
}

static void watch_paths_deinit() {
	int i;
	for (i = 0; i < watch_paths.max_sz; i++) {
		if (watch_paths.paths[i] != NULL) {
			free(watch_paths.paths[i]);
		}
	}
	free(watch_paths.paths);
}

static char *watch_paths_get(int wd) {
	return watch_paths.paths[wd];
}

static void watch_paths_put(int wd, const char *path) {
	int newsz;
	
	if (watch_paths.max_sz <= wd) {
		newsz = watch_paths.max_sz + ARRAY_BLOCK_SZ;
		if (wd + 1 >= newsz) {
			newsz = wd + 1;
		}
		watch_paths.paths = realloc(watch_paths.paths, newsz * sizeof (char *));
		printf("Increasing size from %d, to %d\n",
			watch_paths.max_sz, newsz);
		memset(watch_paths.paths + watch_paths.max_sz, 0, 
			(newsz - watch_paths.max_sz) * sizeof (char *));
		watch_paths.max_sz = newsz; 
	} else if (watch_paths.paths[wd] != NULL) {
		free(watch_paths.paths[wd]);
	}
	watch_paths.paths[wd] = malloc(strlen(path) + 1);
	strcpy(watch_paths.paths[wd], path);
}

static int watchdir(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	int wd;
	if (typeflag == FTW_D) {
		printf("Adding watch to directory `%s`\n", fpath);
		if ((wd = inotify_add_watch(inotify_fd, fpath, IN_ALL_EVENTS)) == -1) {
			perror("Failed to add watch on directory");
		} else {
			watch_paths_put(wd, fpath);
		}
	}
	return 0;
}

static void handle_event(struct inotify_event *ev) {
	char *ename, *times, pathbuf[PATH_MAX], *tabs;
	time_t timet;
	int offset;

	if (ev->mask & IN_ACCESS)
		ename = "IN_ACCESS";
	else if (ev->mask & IN_ATTRIB)
		ename = "IN_ATTRIB";
	else if (ev->mask & IN_CLOSE_WRITE)
		ename = "IN_CLOSE_WRITE";
	else if (ev->mask & IN_CLOSE_NOWRITE)
		ename = "IN_CLOSE_NOWRITE";
	else if (ev->mask & IN_CREATE)
		ename = "IN_CREATE";
	else if (ev->mask & IN_DELETE)
		ename = "IN_DELETE";
	else if (ev->mask & IN_DELETE_SELF)
		ename = "IN_DELETE_SELF";
	else if (ev->mask & IN_MODIFY)
		ename = "IN_MODIFY";
	else if (ev->mask & IN_MOVE_SELF)
		ename = "IN_MOVE_SELF";
	else if (ev->mask & IN_MOVED_FROM)
		ename = "IN_MOVED_FROM";
	else if (ev->mask & IN_MOVED_TO)
		ename = "IN_MOVED_TO";
	else if (ev->mask & IN_OPEN)
		ename = "IN_OPEN";
	else if(ev->mask & IN_IGNORED)
		ename = "IN_IGNORED";
	else 
		ename = "UNKNOWN";
	if (strlen(ename) <= 10) {
		tabs = "\t\t";
	} else {
		tabs = "\t";
	}
	time(&timet);
	times = ctime(&timet);
	times[24] = '\0';
	if (ev->len > 1) {
		printf("[%s] Event: '%s'%sName: '%s'\n", times, ename, tabs, ev->name);
	} else {
		printf("[%s] Event: '%s'\n", times, ename);
	}
	// Note that IN_MOVE_ stays watching the file, so in that case there's
	// no need to re-add the watch.
	if (ev->mask & IN_ISDIR && ev->mask & IN_CREATE) {
		sprintf(pathbuf, "%s/%s", watch_paths_get(ev->wd), ev->name);
		watchdir(pathbuf, NULL, FTW_D, NULL);
	}
}

int main(int argc, char **argv) {
	struct inotify_event *inev;
	char buf[BUFSZ] __attribute__((aligned(8)));
	char *bufp, **watch_dirs;
	int nread;
	
	if (argc < 2)
		error("Usage: inotify_usage <dir>");
	if ((inotify_fd = inotify_init()) == -1)
		error("inotify_init");
	if (inotify_add_watch(inotify_fd, argv[1], IN_ALL_EVENTS | IN_ONLYDIR) == -1) {
		if (errno == ENOTDIR)
			error("The supplied argument is not a directory.");
		else
			error("inotify_add_watch");
	}
	watch_paths_init();
	if (nftw(argv[1], watchdir, 20, 0) == -1)
		error("nftw");
	// forever, read fd, respond
	while (1) {
		if ((nread = read(inotify_fd, buf, BUFSZ)) < 1)
			error("read");
		for (bufp = buf; bufp < buf + nread; 
			bufp += sizeof (struct inotify_event) + inev->len) {
			inev = (struct inotify_event *)bufp;
			handle_event(inev);
		}
	}
	watch_paths_deinit();
	close(inotify_fd);
	exit(EXIT_SUCCESS);
}
