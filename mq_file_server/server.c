#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>
#include <signal.h>
#include <syslog.h>


#include "server.h"

static int qid;

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
	mq_unlink(MQ_PATH);
	mq_close(qid);
	closelog();
	signal(s, SIG_DFL);
	raise(s);
	syslog(LOG_ERR, "Failed to stop by re-raising sig %d", s);
	exit(EXIT_FAILURE);
}

static void setup_sig() {
	struct sigaction sigact;
	sigemptyset(&sigact.sa_mask);
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

static void create_mq() {
	struct mq_attr mattr;
	mattr.mq_maxmsg = 10;
	mattr.mq_msgsize = sizeof (union file_msg);
	if ((qid = mq_open(MQ_PATH, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXO, &mattr)) == (mqd_t)-1)
		exit_err("create_mq mq_open");
}

static int get_request(union file_msg *msg) {
	if (mq_receive(qid, (char*)msg, sizeof (union file_msg), NULL) == -1)
		return -1;
	return 0;
}

static void handle_request(union file_msg *msg) {
	int fd, n;
	mqd_t mqclient;
	union file_msg res;

	syslog(LOG_INFO, "Serving request for file %s", msg->req.path);
	if ((mqclient = mq_open(msg->req.qpath, O_WRONLY)) == (mqd_t)-1) {
		syslog(LOG_INFO, "Failed to open the clients message queue");
		return;
	}
	if ((fd = open(msg->req.path, O_RDONLY)) == -1) {
		syslog(LOG_INFO, "Client requested path we can't open");
		res.part.type = MSG_OPEN_ERROR;
		if (mq_send(mqclient, (char*)&res, sizeof (union file_msg), 1) == -1)
			syslog(LOG_INFO, "Failed to send MSG_OPEN_ERROR to client");
		return;
	}

	res.part.type = MSG_PART;
	errno = 0;
	while ((n = read(fd, res.part.data, DATASZ)) > 0) {
		res.part.n = n;
		if (mq_send(mqclient, (char*)&res, sizeof (union file_msg), 1) == -1)
			break;
	}
	if (errno != 0) {
		syslog(LOG_INFO, "Failed to read file at %s (%s)\n", msg->req.path,
			strerror(errno));
		res.part.type = MSG_READ_ERROR;
		if (mq_send(mqclient, (char*)&res, sizeof (union file_msg), 1) == -1)
			syslog(LOG_INFO, "Failed to send MSG_READ_ERROR to client");
		return;
	} else {
		res.part.type = MSG_DONE;
		if (mq_send(mqclient, (char*)&res, sizeof (union file_msg), 1) == -1)
			syslog(LOG_INFO, "Failed to send MSG_DONE to client");
	}
}

int main(int argc, char **argv) {
	union file_msg msg;
	int waitr;

	daemonize();
	openlog("file_server", LOG_PID | LOG_PERROR, LOG_DAEMON);
	syslog(LOG_INFO, "Started");
	setup_sig();
	create_mq();
	while (get_request(&msg) != -1) {
		switch (fork()) {
		case -1:
			exit_err("fork!");
		case 0:
			handle_request(&msg);
			// Don't call atexit etc.
			exit(EXIT_SUCCESS);
		default:
			break;
		}
	}
	exit_err("get_request");
}
