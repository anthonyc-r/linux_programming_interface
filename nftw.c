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

#define FTW_ACTIONRETVAL 1
#define FTW_CHDIR 2
#define FTW_DEPTH 4
#define FTW_MOUNT 8
#define FTW_PHYS 16

struct link {
	DIR *dir;
	int path_end;
	long dir_loc;
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

static void freelist(struct link *head) {
	struct link *next, *current;
	next = head;
	while (next != NULL) {
		current = next;
		next = current->next;
		free(current);
	}
}

static DIR *nftw_opendir_path(int depth, int noopenfd, struct link **lclosed, char *path) {
	struct link *toclose;
	if (depth > noopenfd) {
		toclose = (*lclosed)->next;
		if ((toclose->dir_loc = telldir(toclose->dir)) == -1)
			return NULL;
		closedir(toclose->dir);
		toclose->dir = NULL;	
		*lclosed = toclose;
	}
	return opendir(path);
}
static DIR *nftw_reopendir(int depth, int noopenfd, struct link *clink, char *path) {
	DIR *ret;
	if (clink->dir != NULL) {
		ret = clink->dir;
	} else {
		ret = opendir(path);
		seekdir(ret, clink->dir_loc);
	}
	return ret;
} 


// TODO: - Respect noopenfd value
int nftw(const char *dirpath, int (*fn) (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf), int noopenfd, int flags) {
	int fpath_len, statcode, chdir_fd, retval;
	// silence warning, bpath+d_name wont be > PATH_MAX.
	char fpath[PATH_MAX + 256], bpath[PATH_MAX];
	struct stat sb, sb_linkcheck;
	struct FTW ftwb;
	DIR *dp_outer, *dp;
	struct dirent *dirent;
	struct link *head, *clink, *lclosed;
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
	// If FTW_CHDIR, chdir to start..
	if (flags & FTW_CHDIR) {
		if (chdir(dirpath) == -1)
			return -1;
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
				retval = fn(fpath, &sb, FTW_NS, &ftwb);
			} else if ((sb.st_mode & S_IFMT) == S_IFDIR) {
				// FTW_D or FTW_DNR or FTW_DP
				if ((dp = opendir(fpath)) == NULL) {
					// FTW_DNR
					retval = fn(fpath, &sb, FTW_DNR, &ftwb);
				} else {
					// FTW_DP is handled outside of this while
					// (dirent.. ) loop As it leaves the loop having 
					// read all the files at deeper levels.
					// Only if FTW_DEPTH is specified. Else treat inline
					// with other files.
					if (!(flags & FTW_DEPTH)) {
						retval = fn(fpath, &sb, FTW_D, &ftwb);
					}
					// If changed dev and FTW_MOUNT, don't iter through
					// that dir to avoid crossing mount points.
					if (flags & FTW_MOUNT && sb.st_dev != dev) {
						// Since we're done with any processing in
						// this directory.
						if (flags & FTW_DEPTH) {
							retval = fn(fpath, &sb, FTW_DP, &ftwb);
						}
						goto handle_actionret_and_continue;
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
					link->dir_loc = -1;
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
					retval = fn(fpath, &sb, FTW_SLN, &ftwb);
				} else {
					retval = fn(fpath, &sb, FTW_SL, &ftwb);
				}
			} else {
				// FTW_F
				retval = fn(fpath, &sb, FTW_F, &ftwb);
			}
			// Reset errno in case it was set above (dangling link stat)
			errno = 0;
handle_actionret_and_continue:
			// If FTW_ACTIONRETVAL handle the retval.
			if (flags & FTW_ACTIONRETVAL) {
				switch (retval) {
					// Fastforward DIR* to skip siblings. Effectively the
					// same thing at the end of the above loop.
					// Assuming fn doesn't mis-return SKIP_SUBTREE.
					case FTW_SKIP_SIBLINGS:
					case FTW_SKIP_SUBTREE:
						goto finished_current_level;
					case FTW_STOP:
						// Return early. Will have items in the list.
						freelist(head);
						return 0;
					case FTW_CONTINUE:
						// Continue as normal.
						break;
					default:
						// Treat any other values as FTW_CONTINUE.
						break;
				}
			}
		}
		// If errno was set by readdir above something went wrong.
		if (errno != 0) {
			return -1;
		}
finished_current_level:
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
	if (ftwbuf->level > 0 && typeflag == FTW_D) {
		puts("Skip Subtree...");
		return FTW_SKIP_SUBTREE;
	} else if (typeflag == FTW_SLN) {
		puts("Stop!");
		return FTW_STOP;
	}
	return FTW_CONTINUE;
}

int main(int argc, char **argv) {
	if (argc < 2)
		error("usage: nftw <pathname>");
	if (nftw(argv[1], fn, 20,  FTW_PHYS | FTW_CHDIR | FTW_MOUNT 
		| FTW_ACTIONRETVAL) == -1)
		error("nftw");
	exit(EXIT_SUCCESS);
}
