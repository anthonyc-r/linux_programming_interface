/**
 * A replacement for sendfile(2) using read(2), write(2) and lseek(2).
 * Attempts to at least account for EINTR.
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSZ 32

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
	off_t os;
	ssize_t nread, nwrite, total;
	char buf[BUFSZ], *start, *end;

	os = 0;
	if (offset != NULL)
		os = *offset;

	if (lseek(in_fd, os, SEEK_SET) == -1)
		return -1;
	total = count;
	while (total > 0 && ((nread = read(in_fd, buf, BUFSZ)) > 0
			       	|| errno == EINTR)) {

		if (nread == -1) //EINTR
			continue;
		start = buf;
		if (nread > total)
			nread = total;
		end = start + nread;

		while (start != end) {
			if ((nwrite = write(out_fd, buf, nread)) == -1) {
				if (errno == EINTR)
					continue;
				else
					return -1;
			}
			start += nwrite;
		}
		total -= nread;
	}
	return count - total;
}

int main(int argc, char **argv) {
	int fd;

	fd = open(argv[1], O_RDONLY);
	sendfile(STDOUT_FILENO, fd, NULL, 100);	
	exit(EXIT_SUCCESS);
}
