/*
* Verifying the behavior of the RLIMIT_MEMLOCK resource limit
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int psz;
	struct rlimit rlim;
	char *buf;

	psz = sysconf(_SC_PAGESIZE);
	if (getrlimit(RLIMIT_MEMLOCK, &rlim) == -1)
		exit_err("getrlimit");
	if (rlim.rlim_cur > psz) {
		puts("Lowering RLIMIT_MEMLOCK");
		rlim.rlim_cur = psz;
		if (setrlimit(RLIMIT_MEMLOCK, &rlim) == -1)
			exit_err("setrlimit");
	}
	printf("RLIMIT_MEMLOCK is %d\n", (int)rlim.rlim_cur);
	// For our purposes the memory needs to be page aligned
	// Else we will end up locking 2 pages instead of 1 (locked mem is
	// rounded down and up by whole page).
	if (posix_memalign((void**)&buf, psz, psz) == -1)
		exit_err("posix_memalign");

	printf("Attempting to mlock %d bytes\n", psz + 1);
	if (mlock(buf, psz + 1) == -1)
		printf("mlock failed (as expected): %s\n", strerror(errno));
	printf("Attempting to mlock %d bytes\n", psz);
	if (mlock(buf, psz) == -1)
		exit_err("mlock");
	puts("Successfully locked!");
	exit(EXIT_SUCCESS);
}
