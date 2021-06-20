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
#include <mqueue.h>

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
		mq_unlink(MQ_SERVER);
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
	int smq, cmq, bak, ok;
	char cpath[PATH_MAX];
	struct num_msg msg;
	struct sigaction sigact;
	struct mq_attr mqattr;

	mqattr.mq_maxmsg = 10;
	mqattr.mq_msgsize = sizeof (struct num_msg);

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

	mq_unlink(MQ_SERVER);

	if ((smq = mq_open(MQ_SERVER, O_CREAT | O_RDONLY, FILE_PERMS, &mqattr)) == -1)
		exit_err("mq_open");

	errno = 0;
	while ((ok = mq_receive(smq, (char *)&msg, sizeof (struct num_msg), NULL)) != -1) {
		printf("Got request - pid: %lld, nreq: %d\n", (long long)msg.pid, msg.n);
		for (i = 0; i < msg.n; i++) {
			msg.nums[i] = n++;
		}
		if (snprintf(cpath, PATH_MAX, MQ_CLIENT_TEMPLATE, (long long)msg.pid) < 0)
			exit_err("snprintf");

		if ((cmq = mq_open(cpath, O_WRONLY, FILE_PERMS)) == -1) {
			perror("Cannot open client message queue, skipping");
			break;
		}
		// Send size, then actual msg
		if (mq_send(cmq, (char *)&msg, sizeof (struct num_msg), 1) != -1) {
			update_backup(bak, n);
		}
		mq_close(cmq);
	}
	if (ok == -1)
		exit_err("mq_receive");

	close(bak);
	mq_close(smq);
	unlink(MQ_SERVER);
	exit(EXIT_SUCCESS);
}
