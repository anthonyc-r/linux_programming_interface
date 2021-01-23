/**
* This program opens a pipe, forks, then sends n blocks of size through the pipe.
* Upon finishing it prints data sent, time elapsed, and data transfer rate.
* ./pipes <nblocks> <blocksz>
* 
* Example Output:
* netty$ ./pipes 10 10
* netty$ Read 100 bytes in blocks of 10
* In 0.00 seconds
* Transfer Rate: 1.17 MB/s
* 
* netty$ ./pipes
* netty$ ./pipes 1000 10
* netty$ Read 10000 bytes in blocks of 10
* In 0.00 seconds
* Transfer Rate: 6.47 MB/s
* 
* netty$ ./pipes 10000 100000
* netty$ Read 1000000000 bytes in blocks of 100000
* In 1.40 seconds
* Transfer Rate: 716.41 MB/s
* 
* netty$ ./pipes 1000000 1000
* Read 1000000000 bytes in blocks of 1000
* netty$ In 5.35 seconds
* Transfer Rate: 186.84 MB/s
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

static void exit_usage() {
	puts("./pipes <nblocks> <blocksz>");
	exit(EXIT_FAILURE);
}
static void exit_err(char *reason) {
	printf("%s\n", reason);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int fds[2], nblock, blocksz;
	int nread, i, sumread, maxread;
	char *ep, *buf;
	pid_t child;
	struct timespec tstart, tend;
	double tdelta, rate;

	if (argc < 3)
		exit_usage();
	nblock = strtol(argv[1], &ep, 10);
	if (*ep != '\0')
		exit_usage();
	blocksz = strtol(argv[2], &ep, 10);
	if (*ep != '\0')
		exit_usage();

	if (pipe(fds) == -1)
		exit_err("pipe");
	if ((buf = malloc(blocksz)) == NULL)
		exit_err("malloc");
	maxread = blocksz * nblock;
	alarm(10);
	switch (fork()) {
		case -1:
			exit_err("fork");
		case 0:
			if (clock_gettime(CLOCK_MONOTONIC, &tstart) == -1)
				exit_err("clock_gettime");
			sumread = 0;
			while ((nread = read(fds[1], buf, blocksz)) > 0) {
				sumread += nread;
				if (sumread >= maxread)
					break;
			}
			if (nread == -1)
				exit_err("read");
			if (clock_gettime(CLOCK_MONOTONIC, &tend) == -1)
				exit_err("clock_gettime");
			tdelta = tend.tv_nsec - tstart.tv_nsec;
			tdelta /= 1000000000.0;
			tdelta += tend.tv_sec - tstart.tv_sec;
			rate = maxread / tdelta;
			printf("Read %d bytes in blocks of %d\n", maxread, blocksz);
			printf("In %.2f seconds\n", tdelta);
			if (rate < 1000) {
				printf("Transfer Rate: %.2f B/s\n", rate);
			} else if (rate < 1000000) {
				printf("Transfer Rate: %.2f KB/s\n", rate / 1000.0);
			} else {
				printf("Transfer Rate: %.2f MB/s\n", rate / 1000000.0);
			}
		default:
			for (i = 0; i < nblock; i++) {
				if (write(fds[0], buf, blocksz) == -1) {
					kill(child, SIGINT);
					exit_err("write");
				}
			}
	}
	exit(EXIT_SUCCESS);
}
