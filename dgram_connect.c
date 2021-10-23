/**
 * Test what happens when you attempt to send to a datagram socket which has
 * already set it's peer via 'connect'. Note that having called 'connect'
 * on a unix domain datagram socket it is only possible to read and write
 * from it's peer.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define NAMEA "sockA"
#define NAMEB "sockB"
#define NAMEC "sockC"

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void set_addr(struct sockaddr_un *addr, char *name) {
	memset(addr, 0, sizeof (struct sockaddr_un));
	addr->sun_family = AF_UNIX;
	// Linux-specific abstract namespace
	strncpy(&addr->sun_path[1], name, 80);
}

static int setup_sock(char *name) {
	struct sockaddr_un addr;
	int fd;

	set_addr(&addr, name);
	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		exit_err("socket");
	if (bind(fd, (struct sockaddr*)&addr, sizeof (struct sockaddr_un)) == -1)
		exit_err("bind");
	return fd;
}

static void sockA() {
	int fd;
	struct sockaddr_un baddr;
	
	fd = setup_sock(NAMEA);
	set_addr(&baddr, NAMEB);
	if (connect(fd, (struct sockaddr*)&baddr, sizeof (struct sockaddr_un)) == -1)
		exit_err("connect a->b");
	pause();
	puts("A exiting");
}

static void sockB() {
	int fd;

	fd = setup_sock(NAMEB);
	pause();
	puts("B exiting");
}

static void sockC() {
	int fd;
	struct sockaddr_un aaddr;
	char msg[1];

	fd = setup_sock(NAMEC);
	set_addr(&aaddr, NAMEA);
	if (sendto(fd, msg, 1, 0, (struct sockaddr*)&aaddr, sizeof (struct sockaddr_un)) == -1) 
		exit_err("sendto c->a");
	puts("C exiting");
}


int main(int argc, char **argv) {
	pid_t a, b;


	switch((b = fork())) {
	case -1:
		exit_err("fork2");
	case 0:
		sockB();
	default:
		break;
	}
	
	sleep(1);
	switch ((a = fork())) {
	case -1:
		exit_err("fork1");
	case 0:
		sockA();
	default:
		break;
	}

	sleep(1);
	sockC();
	kill(a, SIGINT);
	kill(b, SIGINT);

	exit(EXIT_SUCCESS);
}
