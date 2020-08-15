/**
An implementation of nftw.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

#define FTW_ACTIONRETVAL 0
#define FTW_CHDIR 1
#define FTW_DEPTH 2
#define FTW_MOUNT 4
#define FTW_PHYS 8

struct link {
	DIR *dir;
	int path_end;
	struct link *prev;
	struct link *next;	
};

struct FTW {
	int base;
	int level;
};

enum FTW_FTYPE {
	FTW_F,
	FTW_D, 
	FTW_DNR, 
	FTW_DP, 
	FTW_NS, 
	FTW_SL, 
	FTW_SLN
};

enum FTW_ACTION_RET {
	FTW_CONTINUE,
	FTW_SKIP_SIBLINGS,
	FTW_SKIP_SUBTREE,
	FTW_STOP
};

void iterate_through(const char *dir, int flags) {
	
}

int nftw(const char *dirpath, int (*fn) (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf), int noopenfd, int flags) {
	int base, level, fdcount, fpath_len;
	// silence warning, bpath+d_name wont be > PATH_MAX.
	char fpath[PATH_MAX + 256], bpath[PATH_MAX];
	struct stat sb;
	struct FTW ftwb;
	DIR *dp_outer, *dp;
	struct dirent *dirent;
	struct link head;
	struct link *clink;
	
	// Set up data for current depth
	level = 0;
	fdcount = 1;
	if (realpath(dirpath, bpath) == NULL)
		return -1;
	// Consider FTW_DNR, then return.
	if ((dp_outer = opendir(dirpath)) == NULL)
		return -1;
	head.dir = dp_outer;
	head.next = NULL;
	head.prev = NULL;
	head.path_end = strnlen(bpath, PATH_MAX);
	clink = &head;
	while (clink != NULL) {
		// Iterate through entries at this depth.
		errno = 0;
		while((dirent = readdir(clink->dir)) != NULL) {
			puts("startloop");
			// Skip .. and .
			if (strncmp(dirent->d_name, "..", 3) == 0 
				|| strncmp(dirent->d_name, ".", 2) == 0) {
				continue;
			}
			puts(bpath);
			fpath_len = clink->path_end + strnlen(dirent->d_name, PATH_MAX) + 1;
			strncpy(fpath, bpath, clink->path_end);
			strncpy(fpath + clink->path_end + 1, dirent->d_name, PATH_MAX);
			fpath[clink->path_end] = '/';
			fpath[PATH_MAX - 1] = '\0';
			puts(fpath);
			puts(dirent->d_name);
			ftwb.level = level;
			ftwb.base = clink->path_end;
			if (stat(fpath, &sb) == -1) {
				// FTW_NS.
				fn(fpath, &sb, FTW_NS, &ftwb);
			} else if ((sb.st_mode & S_IFMT) == S_IFDIR) {
				// FTW_D or FTW_DNR or FTW_DP
				printf("now opening %s\n", fpath);
				if ((dp = opendir(fpath)) == NULL) {
					// FTW_DNR
					fn(fpath, &sb, FTW_DNR, &ftwb);
				} else {
					// FTW_D or FTW_DP
					// TODO: - tweak behaviour on FTW_DEPTH
					puts("DIR");
					fn(fpath, &sb, FTW_D, &ftwb);
					struct link *link = malloc(sizeof (struct link));
					clink->next = link;
					link->prev = clink;
					link->next = NULL;
					link->dir = dp;
					link->path_end = strnlen(fpath, PATH_MAX);
					clink = link;
					strncpy(bpath, fpath, PATH_MAX);
					level += 1;
					continue;
				}
			} else if ((sb.st_mode & S_IFMT) == S_IFLNK) {
				// FTW_SL or FTW_SLN IF FTW_PHYS!
			} else {
				// FTW_F
				fn(fpath, &sb, FTW_F, &ftwb);
			}
		}
		if (errno != 0) {
			return -1;
		}
		// TODO: - maxfd management?
		closedir(clink->dir);
		clink = clink->prev;
		// Will free head? malloc head too.
		free(clink->next);
		clink->next = NULL;
	}
	
	// TODO: - Free list
	return 0;
}


void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	puts(fpath);
}

int main(int argc, char **argv) {
	if (argc < 2)
		error("usage: nftw <pathname>");
	nftw(argv[1], fn, 20, 0);
	exit(EXIT_SUCCESS);
}
