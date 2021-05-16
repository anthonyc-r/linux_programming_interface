/*
* https://github.com/anthonyc-r/linux_programming_interface
* A simple file copy utility utilizing memory mapping.
*/
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static off_t fsize(int f) {
	struct stat sb;
	if (fstat(f, &sb) == -1)
		return (off_t)-1;
	return sb.st_size;
}

int main(int argc, char **argv) {
	int fsrc, fdest;
	off_t size;
	char *sbuf, *dbuf;

	if (argc < 3) {
		perror("Usage: cp src dest");
		exit(EXIT_FAILURE);
	}
	if ((fsrc = open(argv[1], O_RDONLY)) == -1)
		exit_err("open src");
	if ((fdest = open(argv[2], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		exit_err("open dest");
	if ((size = fsize(fsrc)) == -1)
		exit_err("get file size");	
	if (ftruncate(fdest, size) == -1)
		exit_err("ftruncate");

	if ((sbuf = mmap(NULL, (size_t)size, PROT_READ, MAP_PRIVATE, fsrc, 0)) == (void*)-1)
		exit_err("mmap sbuf");
	if ((dbuf = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_SHARED, fdest, 0)) == (void*)-1)
		exit_err("mmap dbuf");
	close(fsrc);
	close(fdest);

	memcpy(dbuf, sbuf, (size_t)size);
	if (msync(dbuf, (size_t)size, MS_SYNC) == -1)
		exit_err("msync");

	exit(EXIT_SUCCESS);
}
