#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>

#include "server.h"

static int mid;

static void exit_err(char *reason) {
	printf("%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void remove_id() {
	puts("Removing ID");
	msgctl(mid, IPC_RMID, 0);
}

int main(int argc, char **argv) {
	int sid, fd;
	int rcv;
	char *outfile;
	struct file_request req;
	struct file_part part;
	struct stat sb;
	
	if (argc < 2) {
		puts("usage: ./client <filepath>");
		exit(EXIT_FAILURE);
	}
	
	if ((fd = open(ID_FILE, O_RDONLY)) == -1)
		exit_err("open - is server running?");
	if (read(fd, &sid, sizeof (int)) != sizeof(int))
		exit_err("Invalid id file");
	close(fd);
	
	if ((outfile = basename(argv[1])) == NULL)
		exit_err("basename");
	if (stat(outfile, &sb) != -1)
		exit_err("output file exists - not overwriting");
	if ((fd = open(outfile, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR )) == -1)
		exit_err("open O_CREAT");
		
		
	if ((mid = msgget(IPC_PRIVATE, S_IRWXU | S_IRWXO)) == -1)
		exit_err("msgget");
	atexit(remove_id);
		
	req.type = MSG_REQ;
	req.qid = mid;
	strncpy(req.path, argv[1], PATH_MAX);
	if (msgsnd(sid, &req, REQSZ, 0) == -1)
		exit_err("msgsnd");
	while ((rcv = msgrcv(mid, &part, PARTSZ, 0, 0)) != -1) {
		switch (part.type) {
		case MSG_PART:
			if (write(fd, part.data, part.n) != part.n)
				exit_err("write");
			printf("MSG_PART|");
			break;
		case MSG_DONE:
			puts("MSG_DONE");
			goto done;
		case MSG_OPEN_ERROR:
			exit_err("Server sent MSG_OPEN_ERROR");
		case MSG_READ_ERROR:
			exit_err("Server sent MSG_READ_ERROR");
		default:
			break;
		}
	}
done:
	close(fd);
	if (rcv == -1)
		exit_err("msgrcv");

	exit(EXIT_SUCCESS);
}
