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

// TODO: - Handle ACTIONRETVAL
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

// TODO: - Handle fn returns.
enum FTW_ACTION_RET {
	FTW_CONTINUE,
	FTW_SKIP_SIBLINGS,
	FTW_SKIP_SUBTREE,
	FTW_STOP
};


// TODO: - Respect noopenfd value
int nftw(const char *dirpath, int (*fn) (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf), int noopenfd, int flags) {
	int fpath_len, statcode, chdir_fd;
	// silence warning, bpath+d_name wont be > PATH_MAX.
	char fpath[PATH_MAX + 256], bpath[PATH_MAX];
	struct stat sb, sb_linkcheck;
	struct FTW ftwb;
	DIR *dp_outer, *dp;
	struct dirent *dirent;
	struct link *head, *clink;
	dev_t dev;
	int (*statfn) (const char *pathname, struct stat *statbuf);
	
	// We use lstat instead of stat if FTW_PHYS is specified to enable the 
	// `else if ((sb.st_mode & S_IFMT) == S_IFLNK)` branch below, else stat just follows
	// links.
	if (flags & FTW_PHYS) {
		statfn = lstat;
	} else {
		statfn = stat;
	}
	// If FTW_MOUNT, save the starting device ID so we can make sure we don't cross
	// to another mounted filesystem.
	if (flags & FTW_MOUNT) {
		if (stat(dirpath, &sb) == -1)
			return -1;
		dev = sb.st_dev;
	}
	
	// Set up data for current depth
	if (realpath(dirpath, bpath) == NULL)
		return -1;
	// Can't open the dirpath, return error
	if ((dp_outer = opendir(dirpath)) == NULL)
		return -1;
	// Setup ftwb for level 0
	ftwb.level = 0;
	ftwb.base = strnlen(dirpath, PATH_MAX);
	// Use a linked list of DIR* to keep track of where we're at in the tree walk
	// Just convenient here to use a linked list, consider an array to avoid malloc in
	// the main loop.
	head = malloc(sizeof (struct link));
	head->dir = dp_outer;
	head->next = NULL;
	head->prev = NULL;
	head->path_end = strnlen(bpath, PATH_MAX);
	clink = head;
	// While the list of our directories isn't empty, iterate through the last DIR* using
	// readdir.
	while (clink != NULL) {
		// Iterate through entries at this depth. If we encounter a directory then
		// that directory is appended to our list and clink is set to point to it.
		// Else call back into fn appropriately.
		errno = 0;
		while((dirent = readdir(clink->dir)) != NULL) {
			// Skip .. and . to avoid an infinite loop. 
			// As an improvement, consider sym link loops!?
			if (strncmp(dirent->d_name, "..", 3) == 0 
				|| strncmp(dirent->d_name, ".", 2) == 0) {
				continue;
			}
			// Set up the fpath to contain the full path of the current file
			fpath_len = clink->path_end + strnlen(dirent->d_name, PATH_MAX) + 1;
			strncpy(fpath, bpath, clink->path_end);
			strncpy(fpath + clink->path_end + 1, dirent->d_name, PATH_MAX);
			fpath[clink->path_end] = '/';
			fpath[PATH_MAX - 1] = '\0';
			// ftwb.base is set to the length of the directory the item is in
			// i.e. /path/to/file -> 9
			ftwb.base = clink->path_end;
			if (statfn(fpath, &sb) == -1) {
				// FTW_NS.
				fn(fpath, &sb, FTW_NS, &ftwb);
			} else if ((sb.st_mode & S_IFMT) == S_IFDIR) {
				// FTW_D or FTW_DNR or FTW_DP
				if ((dp = opendir(fpath)) == NULL) {
					// FTW_DNR
					fn(fpath, &sb, FTW_DNR, &ftwb);
				} else {
					// FTW_DP is handled outside of this while
					// (dirent.. ) loop As it leaves the loop having 
					// read all the files at deeper levels.
					// Only if FTW_DEPTH is specified. Else treat inline
					// with other files.
					if (!(flags & FTW_DEPTH)) {
						fn(fpath, &sb, FTW_D, &ftwb);
					}
					// If it's ino is 1, then it's the root of a new fs
					// if FTW_MOUNT is set we don't want to cross over
					// to this new file system!
					if (flags & FTW_MOUNT && sb.st_dev != dev) {
						// Since we're done with any processing in
						// this directory.
						if (flags & FTW_DEPTH) {
							fn(fpath, &sb, FTW_DP, &ftwb);
						}
						continue;
					}
					// chdir into it if need to
					if (flags & FTW_CHDIR) {
						chdir(fpath);
					}
					// Set up a new item in the list and set it as the 
					// current item/link to be readdir'd over 
					struct link *link = malloc(sizeof (struct link));
					clink->next = link;
					link->prev = clink;
					link->next = NULL;
					link->dir = dp;
					link->path_end = strnlen(fpath, PATH_MAX);
					// clink is updated, so when we continue the loop,
					// this updated dir will be iterated over.
					clink = link;
					strncpy(bpath, fpath, PATH_MAX);
					// Keep ftwb.level up to date. It's now one deeper.
					ftwb.level++;
				}
			} else if ((sb.st_mode & S_IFMT) == S_IFLNK) {
				// FTW_SL or FTW_SLN
				// Just stat the link to check if it's dangling.
				if (stat(fpath, &sb_linkcheck) == -1) {
					fn(fpath, &sb, FTW_SLN, &ftwb);
				} else {
					fn(fpath, &sb, FTW_SL, &ftwb);
				}
			} else {
				// FTW_F
				fn(fpath, &sb, FTW_F, &ftwb);
			}
			// Reset errno in case it was set above (dangling link stat)
			errno = 0;
		}
		// If errno was set by readdir above something went wrong.
		if (errno != 0) {
			return -1;
		}
		// Everything at this deeper level has been iterated over, so we move back 
		// up a level.
		ftwb.level--;
		// As we've finished iterating through this dir and all deeper, close it and
		// free the list item.
		closedir(clink->dir);
		clink = clink->prev;
		if (clink != NULL) {
			free(clink->next);
			clink->next = NULL;
		} 
		// If clink is NULL here, we're at the topmost level and have finished.
		
		// If FTW_DEPTH is set, then we skipped fn for this file above, so we now go
		// and call fn for it. Have to stat it again, as sb won't be set for this
		// file.
		// First update our cwd if ness... 
		if (clink == NULL) {
			if (chdir(dirpath) == -1 || chdir("..") == -1)
				return -1;
		} else {
			if ((chdir_fd = dirfd(clink->dir)) == -1)
				return -1;
			if (fchdir(chdir_fd) == -1)
				return -1;
		}
		if (flags & FTW_DEPTH) {
			bpath[ftwb.base] = '\0';
			if (statfn(bpath, &sb) == -1) {
				fn(bpath, &sb, FTW_NS, &ftwb);
			} else {
				fn(bpath, &sb, FTW_DP, &ftwb);
			}
		}
	}
	return 0;
}


void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	char buf[PATH_MAX];
	puts("==========================================");
	for (int i = -1; i < ftwbuf->level; i++) {
		printf("*");
	}
	printf("\n");
	printf("(%s)\n", fpath);
	printf("[%s]\n", getcwd(buf, PATH_MAX));
	switch (typeflag) {
		case FTW_F:
			puts("File");
			break;
		case FTW_D:
			puts("Dir");
			break;
		case FTW_DP:
			puts("Dir (Depth)");
			break;
		case FTW_SL:
			puts("Symlink");
			break;
		case FTW_SLN:
			puts("Symlink (Dangling)");
			break;
	}
	puts("==========================================");
}

int main(int argc, char **argv) {
	if (argc < 2)
		error("usage: nftw <pathname>");
	if (nftw(argv[1], fn, 20, FTW_DEPTH | FTW_PHYS | FTW_CHDIR | FTW_MOUNT) == -1)
		error("nftw");
	exit(EXIT_SUCCESS);
}
