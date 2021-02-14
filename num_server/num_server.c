/*
* Serve sequential numbers to clients.
* Client opens FIFO_CLIENT_TEMPLATE with it's pid as a parameter, then writes a num_open_req
* structure to the file FIFO_SERVER. The server responds by writing num_response to
* FIFO_CLIENT_TEMPLATE(pid).
*/

#include "num_server.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>

#define REQ_SZ sizeof (struct num_open_req)
#define BAK_FILE "/tmp/num_server.bak"
#define FILE_PERMS S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH

static volatile sig_atomic_t timeout;

static void exit_err(char *reason) {
	printf("%s (%s)\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static int load_backup(int *seq_start) {
	int fd, flags;
	struct stat sbuf;
	off_t off;

	*seq_start = 0;
	flags = O_RDWR | O_SYNC;

	if (stat(BAK_FILE, &sbuf) == -1) {
		puts("stat -1");
		flags |= O_CREAT;
	}
	if ((fd = open(BAK_FILE, flags, FILE_PERMS)) == -1)
		exit_err("open backup file");
	// New file, starts at 0.
	if (flags & O_CREAT) {
		puts("Creating new backup file");
		if (write(fd, seq_start, sizeof (int)) != sizeof (int))
			exit_err("write init backup");
		return fd;
	}

	// Make sure the existing file is valid.
	if (lseek(fd, 0, SEEK_END) != sizeof (int)) {
		exit_err("lseek invalid backup file");
	}

	// Ok.
	off = lseek(fd, 0, SEEK_SET);
	if (off == (off_t) - 1)
		exit_err("lseek 2");
	if (read(fd, seq_start, sizeof (int)) != sizeof (int))
		exit_err("read backup");
	printf("backup is %d\n", *seq_start);
	return fd;
}

static void update_backup(int fd, int seq_start) {
	if (lseek(fd, 0, SEEK_SET) == (off_t) -1)
		exit_err("lseek 3");
	printf("bakfd: %lld\n", (long long)fd);
	if (write(fd, &seq_start, sizeof (int)) != sizeof (int))
		exit_err("write backup");
}

void handler(int sig) {
	switch (sig) {
	case SIGTERM:
	case SIGINT:
		unlink(FIFO_SERVER);
		exit(EXIT_SUCCESS);
	case SIGALRM:
		timeout = 1;
		// Not safe to use stdio but...
		puts("timeout");
		break;
	default:
		break;
	}
}

int main(int argc, char **argv) {
	int n, i;
	int sfifo, cfifo, sfifo_write, bak;
	char cpath[PATH_MAX];
	ssize_t nread, res_sz;
	struct num_open_req req;
	struct num_response *res;
	struct sigaction sigact;
	
	sigact.sa_handler = handler;
	if (sigemptyset(&sigact.sa_mask) == -1)
		exit_err("sigemptyset");
	if (sigaction(SIGINT, &sigact, NULL) == -1)
		exit_err("sigaction 1");
	if (sigaction(SIGTERM, &sigact, NULL) == -1)
		exit_err("sigaction 2");
	if (sigaction(SIGALRM, &sigact, NULL) == -1)
		exit_err("sigaction 3");

	bak = load_backup(&n);
	
	unlink(FIFO_SERVER);
	if (mkfifo(FIFO_SERVER, FILE_PERMS) == -1)
		exit_err("mkfifo");
	if ((sfifo = open(FIFO_SERVER, O_RDONLY)) == -1)
		exit_err("open 1");
	
	// Avoid ever getting EOF from read(sfifo)
	if ((sfifo_write = open(FIFO_SERVER, O_WRONLY)) == -1)
		exit_err("open 2");

	errno = 0;
	while ((nread = read(sfifo, &req, REQ_SZ)) == REQ_SZ) {
		printf("Got request - pid: %lld, nreq: %d\n", (long long)req.pid, req.nreq);
		res_sz = (sizeof (struct num_response)) + req.nreq * sizeof (int);
		if ((res = malloc(res_sz)) == NULL)
			exit_err("malloc 1");
		res->nres = req.nreq;
		for (i = 0; i < req.nreq; i++) {
			res->nums[i] = n++;
		}
		if (snprintf(cpath, PATH_MAX, FIFO_CLIENT_TEMPLATE, (long long)req.pid) < 0)
			exit_err("snprintf");

		// Allow the client 1 second to open it's FIFO, to avoid DOS attacks.
		alarm(1);
		errno = 0;
		if ((cfifo = open(cpath, O_WRONLY)) == -1) {
			// Client hasn't opened it's end of the FIFO!!
			if (errno == EINTR && timeout) {
				timeout = 0;
				puts("Misbehaved client, skipping");
				continue;
			} else {
				// Unexpected error.
				exit_err("open 3");
			}
		}
		alarm(0);

		// Possibility that a misbehaved client has closed it's end of the FIFO. Handle EPIPE
		errno = 0;
		if (write(cfifo, &res_sz, sizeof (int)) != sizeof  (int)) {
			if (errno == EPIPE) {
				puts("Misbehaved client, skipping");
				continue;
			} else {
				exit_err("write 1");
			}
		}
		errno = 0;
		if (write(cfifo, res, res_sz) != res_sz) {
			if (errno == EPIPE) {
				puts("Misbehaved client, skipping");
				continue;
			} else {
				exit_err("write 2");
			}
		}
		free(res);
		if (close(cfifo) == -1)
			exit_err("close 1");
		update_backup(bak, n);
	}
	if (nread != 0)
		exit_err("read 1");
	
	close(bak);
	close(sfifo_write);
	close(sfifo);
	unlink(FIFO_SERVER);
	exit(EXIT_SUCCESS);
}
