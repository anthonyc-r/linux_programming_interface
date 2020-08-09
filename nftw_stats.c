/**
Uses nftw() to walk through a directory tree and print out the counts
and percentages of the various types (dir, reg, sym, etc).
*/
#define _XOPEN_SOURCE 600

#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

static int syms, regs, dirs, unknowns;

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int count_types(const char *fpath, const struct stat *sb, int typeFlag, struct FTW *ftwbuf) {
	switch (typeFlag) {
		case FTW_F:
			regs++;
			break;
		case FTW_D:
			dirs++;
			break;
		case FTW_DNR:
			dirs++;
			break;
		case FTW_DP:
			dirs++;
			break;
		case FTW_NS:
			unknowns++;
			break;
		case FTW_SL:
			syms++;
			break;
		case FTW_SLN:
			syms++;
			break;
	}
	return 0;
}

void report_stats() {
	int total;
	double pregs, pdirs, psyms, punknowns;
	
	total = regs + dirs + syms + unknowns;
	pregs = 100.0 * regs / total;
	pdirs = 100.0 * dirs / total;
	psyms = 100.0 * syms / total;
	punknowns = 100.0 * syms / total;
	printf("\t\tRegular\t\tDirectory\tSymbolic\tUnknown\n");
	printf("Count:\t\t%d\t\t%d\t\t%d\t\t%d\n", regs, dirs, syms, unknowns);
	printf("Percent:\t%2.1f\t\t%2.1f\t\t%2.1f\t\t%2.1f\n", pregs, pdirs, psyms, punknowns);
}

int main(int argc, char **argv) {
	char path[PATH_MAX];
	
	if (getcwd(path, PATH_MAX) == NULL)
		error("getcwd");
	if(nftw(path, count_types, 20, FTW_PHYS) == -1)
		error("nftw");
	report_stats();
	exit(EXIT_SUCCESS);
}
