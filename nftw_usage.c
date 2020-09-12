/**
Example usage of nftw, noting the difference between file
order when called with and without FTW_DEPTH
*/
#define _XOPEN_SOURCE 600

#include <ftw.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

static int print;

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int print_info(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	if (print) {
		for (int i = 0; i < ftwbuf->level; i++)
			printf("*");
		printf("*\n");
	} else {
		puts("----------------------------------------------");
		switch (typeflag) {
			case FTW_F:
				puts("Regular File: ");
				break;
			case FTW_D:
				puts("Directory: ");
				break;
			case FTW_DNR:
				puts("Directory (Can't Read): ");
				break;
			case FTW_DP:
				puts("Directory (FTW_DEPTH): ");
				break;
			case FTW_NS:
				puts("Can't Stat: (r-- .?): ");
				break;
			case FTW_SL:
				puts("Sym Link (FTW_PHYS): ");
				break;
			case FTW_SLN:
				puts("Dangling Symlink: ");
				break;
		}		
		puts(fpath);
		puts("");
	}
	return 0;
}

int main(int argc, char **argv) {
	char path[PATH_MAX], opt;
	int flags;
	
	flags = 0;
	while ((opt = getopt(argc, argv, "dp")) != -1) { 
		switch (opt) {
			case 'p':
				print = 1;
				break;
			case 'd':
				flags |= FTW_DEPTH;
				break;
			case '?':
				error("usage: nftw_usage [-d] [-p]");
		}
	}
	

	if (getcwd(path, PATH_MAX) == NULL)
		error("getcwd1");	
	nftw(path, print_info, 20, flags);
	exit(EXIT_SUCCESS);
}
