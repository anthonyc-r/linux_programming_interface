/*
* A command line interface to sys v message queues.
* msgctl create <path>
* msgctl delete <id>
* msgctl send -i <id> [num] msg
* msgctl receive -i <id> [num]
*/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

struct msg {
	long type;
	char text[1];
};

static void exit_usage() {
	puts("msgctl create");
	puts("msgctl delete <id>");
	puts("msgctl send  -n<num> <id> <msg>");
	puts("msgctl receive -n<num> <id>");
	exit(EXIT_FAILURE);
}

static void exit_error(char *reason) {
	printf("%s - (%d)%s\n", reason, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static void create() {
	int id;
	
	if ((id = msgget(IPC_PRIVATE, S_IRWXU)) == -1)
		exit_error("msgget");
	printf("%d\n", id);
}

static void delete(int argc, char **argv) {
	int id;
	char *ep;
	
	if (argc != 1)
		exit_usage();
	id = (int)strtol(argv[0], &ep, 10);
	if (*ep != '\0')
		exit_usage();
	if (msgctl(id, IPC_RMID, NULL) == -1)
		exit_error("msgctl");
	printf("Removed %d\n", id);
}

static void send(int num, int argc, char **argv) {
	int id;
	char *ep;
	size_t len;
	struct msg *msg;
	
	if (argc != 2)
		exit_usage();
	id = (int)strtol(argv[0], &ep, 10);
	if (*ep != '\0')
		exit_usage();
	if (num == 0)
		num++;
		
	len = strlen(argv[1]) + 1;
	if ((msg = malloc(sizeof(struct msg) + len)) == NULL)
		exit_error("malloc");
		
	msg->type = num;
	strncpy(msg->text, argv[1], len);
	if (msgsnd(id, msg, len, 0) == -1)
		exit_error("msgsnd");
}

static void receive(int num, int argc, char **argv) {
	int id;
	char *ep;
	size_t len;
	struct msg *msg;
	struct msqid_ds msqbuf;

	if (argc != 1)
		exit_usage();
	id = (int)strtol(argv[0], &ep, 10);
	if (*ep != '\0')
		exit_usage();

	if (msgctl(id, IPC_STAT, &msqbuf) == -1)
		exit_error("msgctl");
	
	msg = calloc(1, msqbuf.msg_qbytes + sizeof(struct msg));
	if (msg == NULL)
		exit_error("malloc");
	if (msgrcv(id, msg, msqbuf.msg_qbytes, num, IPC_NOWAIT) == -1)
		exit_error("msgrcv");
	
	msg->text[msqbuf.msg_qbytes - 1] = '\0';
	printf("%s\n", msg->text);
}

int main(int argc, char **argv) {
	char *operation, *ep;
	int ch, num, has_num;
	
	if (argc < 2)
		exit_usage();
	
	has_num = 0;
	while ((ch = getopt(argc, argv, "n:")) != -1) {
		switch (ch) {
		case 'n':
			has_num = 1;
			num = strtol(optarg, &ep, 10);
			if (*ep != '\0')
				exit_usage();
			break;
		case '?':
			exit_usage();
		}
	}

	operation = argv[1];
	if (!has_num) {
		num = 0;
		optind++;
	}
	argc -= optind;
	argv += optind;
	
	if (strcmp("create", operation) == 0) {
		create();
	} else if (strcmp("delete", operation) == 0) {
		delete(argc, argv);
	} else if (strcmp("send", operation) == 0) {
		send(num, argc, argv);
	} else if (strcmp("receive", operation) == 0) {
		receive(num, argc, argv);
	}
	exit(EXIT_SUCCESS);
}
