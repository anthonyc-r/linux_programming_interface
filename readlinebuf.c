/**
 * A buffered readline implementation. Currently fails if line size exceeds
 * defined BUFSZ. Could fix by either dynamically allocating memory,
 * or to avoid this, allowing the user to pass buffers into the init func.
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFSZ 128

struct rlbuf {
	int fd, eof;
	char buf[BUFSZ], res[BUFSZ], *cur, *ep;
};

int readLineBufInit(int fd, struct rlbuf *rlbuf) {
	int len;

	rlbuf->fd = fd;
	if ((len = read(fd, rlbuf->buf, BUFSZ)) == -1)
		return -1;
	rlbuf->cur = rlbuf->buf;
	rlbuf->ep = rlbuf->buf + len;
	rlbuf->eof = 0;
	return 0;
}

char *readLineBuf(struct rlbuf *rlbuf) {
	char *src, *dest;
	int len;

	if (rlbuf->cur >= rlbuf->ep && rlbuf->eof)
		return NULL;

	dest = rlbuf->res;
	for (;;) {
		src = rlbuf->cur;
		for(len = 0; rlbuf->cur < rlbuf->ep && *rlbuf->cur != '\n';
				rlbuf->cur++, len++)
			;
		if (dest + len + 1 > rlbuf->res + BUFSZ) {
			errno = EFBIG;
			return NULL;
		}
		memcpy(dest, src, len);
		dest += len;

		if (rlbuf->cur < rlbuf->ep || rlbuf->eof) {
			rlbuf->cur++;
			*dest = '\0';
			return rlbuf->res;
		}
		// Need to grab more
		memset(rlbuf->buf, 0, BUFSZ);
		if ((len = read(rlbuf->fd, rlbuf->buf, BUFSZ)) == -1)
			return NULL;
		if (len != BUFSZ)
			rlbuf->eof = 1;
		rlbuf->cur = rlbuf->buf;
		rlbuf->ep = rlbuf->cur + len;
	}

}

int main(int argc, char **argv) {
	struct rlbuf rlbuf;
	int fd;
	char *res;

	fd = open(argv[1], O_RDONLY);
	readLineBufInit(fd,  &rlbuf);
	while ((res = readLineBuf(&rlbuf)) != NULL)
		printf("LINE: '%s'\n", res);
	puts("EOF");
	exit(EXIT_SUCCESS);
}

