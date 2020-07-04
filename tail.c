#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


#ifndef BUFF_SZ
#define BUFF_SZ 100
#endif

#define DEFAULT_NUM 10

void error(char *er) {
	puts(er);
	exit(EXIT_FAILURE);
}

void main(int argc, char **argv) {
	int num, fd, oflags, n, i, numread;
	char buff[BUFF_SZ], *fnam;
	
	
	if (argc < 2) {
		printf("tail [ -n num ] file");
	}
	if (argc > 2) {
		num = (int)strtol(argv[2], NULL, 10);
		fnam = argv[3];
	} else {
		num = DEFAULT_NUM;
		fnam = argv[1];
	}
	
	oflags = O_RDONLY;
	if ((fd = open(fnam, oflags)) == -1)
		error("open\n");
	
	n = 0;
	lseek(fd, 0, SEEK_END);
	while (1) {
		if (lseek(fd, -BUFF_SZ, SEEK_CUR) == -1)
			error("lseek\n");
		errno = 0;
		if ((numread = read(fd, buff, BUFF_SZ)) < 1) {
			if (errno != 0)
				error("read\n");
			break;
		}
		for (i = numread - 1; i >= 0; i--) {
			if (buff[i] == '\n') {
				n++;
			}
			if (n >= num) {
				if (lseek(fd, -(numread - i), SEEK_CUR) == -1)
					error("lseek\n");
				break;
			}
		}
		if (n >= num) {
			break;
		} else if (lseek(fd, -numread, SEEK_CUR) == -1) {
			error("lseek\n");
		}
	}

	posix_fadvise(fd, lseek(fd, 0, SEEK_CUR), -1, POSIX_FADV_SEQUENTIAL);
	errno = 0;
	while ((numread = read(fd, buff, BUFF_SZ)) > 0) {
		if (write(STDOUT_FILENO, buff, numread) != numread)
			error("write\n");
		errno = 0;
	}
	if (errno != 0)
		error("read\n");
	if (close(fd) == -1)
		error("close\n");
		
	exit(EXIT_SUCCESS);
}

