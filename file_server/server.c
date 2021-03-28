#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>
#include <signal.h>
#include <syslog.h>

#include "server.h"

static int id;

static void exit_err(char *reason) {
	syslog(LOG_ERR, "%s - %s\n", reason, strerror(errno));
	exit(EXIT_FAILURE);
}

static void daemonize() {
	switch (fork()) {
	case -1:
		exit_err("fork1");
	case 0:
		if (setsid() == -1)
			exit_err("setsid");
		switch (fork()) {
		case -1:
			exit_err("fork2");
		case 0:
			break;
		default:
			exit(EXIT_SUCCESS);
		}
		
		break;
	default:
		exit(EXIT_SUCCESS);
	}
	if (umask(0) == -1)
		exit_err("umask");
	if (chdir("/") == -1)
		exit_err("chdir");
	close(0);
	if (open("/dev/null", O_RDWR) != 0)
		exit_err("closed 0 & open != 0");
	if (dup2(0, 1) == -1)
		exit_err("dup2 1");
	if (dup2(0, 2) == -1)
		exit_err("dup2 2");
	syslog(LOG_DEBUG, "Became daemon.");
}

static void sig(int s) {
	syslog(LOG_INFO, "Shutting Down - Removing ID");
	unlink(ID_FILE);
	msgctl(id, IPC_RMID, 0);
	closelog();
	signal(s, SIG_DFL);
	raise(s);
	syslog(LOG_ERR, "Failed to stop by re-raising sig %d", s);
	exit(EXIT_FAILURE);
}

static void setup_sig() {
	struct sigaction sigact;
	sigact.sa_mask = 0;
	sigact.sa_flags = SA_NODEFER;
	sigact.sa_handler = sig;
	
	errno = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	signal(SIGCHLD, SIG_IGN);
	
	if (errno != 0)
		exit_err("signal handlers");
}

static void create_id() {
	char *tmp;
	int fd;
	
	if ((id = msgget(IPC_PRIVATE, S_IRWXU | S_IROTH)) == -1)
		exit_err("msgget");
	if ((tmp = tmpnam(NULL)) == NULL)
		exit_err("tmpnam");
	if ((fd = open(tmp, O_WRONLY | O_CREAT, S_IRWXU)) == -1)
		exit_err("open");
	if (write(fd, &id, sizeof(int)) != sizeof(int))
		exit_err("write");
	close(fd);
	if (rename(tmp, ID_FILE) == -1)
		exit_err("rename");
}

static int get_request(struct file_request *req) {
	if (msgrcv(id, req, REQSZ, 0, 0) == -1)
		return -1;
	return 0;
}

static void handle_request(struct file_request *req) {
	int fd;
	int res;
	ssize_t n;
	struct file_part part;
	
	syslog(LOG_INFO, "Serving request for file %s", req->path);
	if ((fd = open(req->path, O_RDONLY)) == -1) {
		syslog(LOG_INFO, "Client requested path we can't open");
		res = MSG_OPEN_ERROR;
		if (msgsnd(req->qid, &res, 0, 0) == -1)
			syslog(LOG_INFO, "Failed to send MSG_OPEN_ERROR to client");
		return;
	}
	
	part.type = MSG_PART;
	errno = 0;
	while ((n = read(fd, part.data, DATASZ)) > 0) {
		part.n = n;
		if (msgsnd(req->qid, &part, PARTSZ, 0) == -1)
			break;
	}
	if (errno != 0) {
		syslog(LOG_INFO, "Failed to read file at %s (%s)\n", req->path,
			strerror(errno));
		res = MSG_READ_ERROR;
		if (msgsnd(req->qid, &res, 0, 0) == -1)
			syslog(LOG_INFO, "Failed to send MSG_READ_ERROR to client");
		return;
	} else {
		res = MSG_DONE;
		if (msgsnd(req->qid, &res, 0, 0) == -1)
			syslog(LOG_INFO, "Failed to send MSG_DONE to client");
	}
}

int main(int argc, char **argv) {
	struct file_request req;
	int waitr;
	
	daemonize();
	openlog("file_server", LOG_PID | LOG_PERROR, LOG_DAEMON);
	syslog(LOG_INFO, "Started");
	setup_sig();
	create_id();
	while (get_request(&req) != -1) {
		switch (fork()) {
		case -1:
			exit_err("fork!");
		case 0:
			handle_request(&req);
			// Don't call atexit etc.
			exit(EXIT_SUCCESS);
		default:
			break;
		}
	}
	exit_err("get_request");
}
