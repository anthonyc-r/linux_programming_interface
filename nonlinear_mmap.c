/*
* Use mmap + the MAP_FIXED flag to map a file in a non-linear way. i.e. page 3, page 1, page 2.
*/

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define TMP_FILE "/tmp/nonlinear_mmap.tmp"

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void pfile(int fd) {
	char buf[50];
	errno = 0;
	while (read(fd, buf, 50) > 0)
		write(STDOUT_FILENO, buf, 50);
	if (errno != 0)
		exit_err("pfile");
	puts("");
}

int main(int argc, char *argv) {
	int fd;
	int psz;
	char *map;
	void *saddr;

	setbuf(stdout, NULL);
	psz = sysconf(_SC_PAGESIZE);
	unlink(TMP_FILE);
	if ((fd = open(TMP_FILE, O_CREAT | O_RDWR, S_IRWXU)) == -1)
		exit_err("open");
	if (ftruncate(fd, 3 * psz) == -1)
		exit_err("ftruncate");
	if ((map = mmap(NULL, 3 * psz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL)
		exit_err("mmap");
	memset(map, 'a', psz);
	memset(map + psz, 'b', psz);
	memset(map + 2 * psz, 'c', psz);
	msync(map, 3 * psz, MS_SYNC);

	puts("Contents of file: ");
	pfile(fd);
	puts("Contents of memory: ");
	write(STDOUT_FILENO, map, 3 * psz);
	puts("");

	saddr = map;
	if (munmap(map, 3 * psz) == -1)
		exit_err("munmap");
	if (mmap(saddr, psz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 2 * psz) == NULL)
		exit_err("mmap 1");
	if (mmap(saddr + psz, psz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, psz) == NULL)
		exit_err("mmap 2");
	if (mmap(saddr + 2 * psz, psz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == NULL)
		exit_err("mmap 3");
	puts("Contents of remapped memory: ");
	write(STDOUT_FILENO, saddr, 3 * psz);
	puts("");
	puts("Writing a*psz b*psz c*pcs to remapped memory...");
	memset(map, 'a', psz);
	memset(map + psz, 'b', psz);
	memset(map + 2 * psz, 'c', psz);
	msync(map, 3 * psz, MS_SYNC);
	// Why isn't the file updated unless we open/close it here? Even though msync has been called.
	close(fd);
	fd = open(TMP_FILE, O_RDWR);
	puts("Contents of file:");
	pfile(fd);
	exit(EXIT_SUCCESS);
}
