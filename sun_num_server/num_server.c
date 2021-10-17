/*
* Serve sequential numbers to clients. This version makes use of 
* unix domain stream sockets.
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
#include <sys/socket.h>
#include <sys/un.h>
#include <setjmp.h>

#define BAK_FILE "/tmp/num_server.bak"
#define FILE_PERMS S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH

sigjmp_buf jenv;

static void exit_err(char *reason) {
	perror(reason);
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
	if (write(fd, &seq_start, sizeof (int)) != sizeof (int))
		exit_err("write backup");
}

void handler(int sig) {
	switch (sig) {
	case SIGALRM:
		siglongjmp(jenv, 1);
	default:
		break;
	}
}

int main(int argc, char **argv) {
	int n, i;
	int sfd, cfd, bak;
	ssize_t nread, res_sz;
	struct sockaddr_un addr;
	struct num_greeting greet;
	struct num_response res;
	struct sigaction sigact;
	

	sigact.sa_handler = handler;
	if (sigemptyset(&sigact.sa_mask) == -1)
		exit_err("sigemptyset");
	if (sigaction(SIGALRM, &sigact, NULL) == -1)
		exit_err("sigaction 3");

	bak = load_backup(&n);
	
	memset(&addr, 0, sizeof (struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(&addr.sun_path[1], SERVER_NAME, 80);
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	if (bind(sfd, (struct sockaddr*)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("bind");
	if (listen(sfd, 10) == -1)
		exit_err("listen");
	
	errno = 0;
	while ((cfd = accept(sfd, NULL, 0)) != -1) {
		// Long jumped from SIGALRM
		if (sigsetjmp(jenv, 1) == 1) {
			perror("Request timed out");
			goto client_end;
		}
		// Allow a total of 1 second to complete the request
		alarm(1);	
		puts("Got request");
		if (read(cfd, &greet, GREETSZ) != GREETSZ) {
			perror("Greeting failed");
			goto client_end;
		}
		if (greet.nreq > MAX_N) {
			perror("Client requesting too many");
			goto client_end;
		}	
		for (i = 0; i < greet.nreq; i++) {
			res.nums[i] = n++;
		}
		res.nums[i] = -1;
		if (write(cfd, &res, RESSZ) != RESSZ) {
			perror("Respond to client failed");
			goto client_end;
		}
client_end:
		if (close(cfd) == -1)
			exit_err("close client");
		alarm(0);
		update_backup(bak, n);
	}
	
	close(bak);
	close(sfd);
	exit(EXIT_SUCCESS);
}
