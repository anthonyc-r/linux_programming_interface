/*
* Verifying the behavior or MADV_DONTNEED on private memory mappings.
* Create a private anonymous memory mapping, mark it with MADV_DONTNEED
* Then check it's value. Linux should have special behavior where it's value
* reinitialized to 0/the file's value.
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void pbuf(char *buf, int len) {
	puts("Contents of buf: ");
	write(STDOUT_FILENO, buf, len);
	puts("");
}

int main(int argc, char **argv) {
	char *buf;
	int psz;

	setbuf(stdout, NULL);
	psz = sysconf(_SC_PAGESIZE);
	if ((buf = mmap(NULL, 2 * psz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == NULL)
		exit_err("mmmap");
	memset(buf, 'a', psz);
	memset(buf + psz, 'b', psz);
	pbuf(buf, 2 * psz);
	puts("madvise MADV_DONTNEED on second half of buf");
	if (madvise(buf + psz, psz, MADV_DONTNEED) == -1)
		exit_err("madvise");
	pbuf(buf, 2 * psz);
	exit(EXIT_SUCCESS);
}
