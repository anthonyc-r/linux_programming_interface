 
/**
 * Based on rsh.c. Allows remotely logging into the system, using a psudoterminal.
 * Usage: ./rsh (server|client host port)
 * Note that this program is quite insecure!
 *
 * Modified to use a pseudoterminal instead of execing sh.
 */
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <poll.h>
#include <utmpx.h>
#include <termios.h>
#include <signal.h>

#define BUFSZ 100

static struct termios tios_restore;

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void restore_tios() {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios_restore);
}

static void goraw() {
	struct termios raw;

	if (tcgetattr(STDOUT_FILENO, &tios_restore) == -1)
		exit_err("tcgetattr");
	if (atexit(restore_tios) != 0)
		exit_err("atexit");
	raw = tios_restore;
	raw.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
	raw.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR |
		INPCK | ISTRIP | IXON | PARMRK);
	raw.c_oflag &= ~OPOST;
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDOUT_FILENO, TCSADRAIN, &raw) == -1)
		exit_err("tcsetattr raw");
}

static void pass_through(int a, int b) {
	ssize_t nread;
	char buf[BUFSZ];
	struct pollfd polls[2];

#define FDA 0
#define FDB 1
	polls[FDA].fd = a;
	polls[FDA].events = POLLIN;
	polls[FDB].fd = b;
	polls[FDB].events = POLLIN;
	while ((nread = poll(polls, 2, -1)) > 0) {
		if (polls[FDA].revents & POLLIN == POLLIN) {
			if ((nread = read(a, buf, BUFSZ)) < 1)
				exit_err("read fd");
			if (write(b, buf, nread) != nread)
				exit_err("write master");
		} else if (polls[FDB].revents & POLLIN == POLLIN) {
			if ((nread = read(b, buf, BUFSZ)) < 1)
				exit_err("read master");
			if (write(a, buf, nread) != nread)
				exit_err("write fd");
		}
	}
#undef FDA
#undef FDB
}

static void client(char *host, char *port, char *cmd) {
	int sock;
	size_t nread;
	struct sockaddr_storage addr;
	struct addrinfo *res, *curs, hints;
	char buf[100];

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		exit_err("socket");
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		exit_err("getaddrinfo");
	for (curs = res; curs != NULL; curs = curs->ai_next) {
		if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
			perror("connect");
			continue;
		} else {
			break;
		}
	}
	if (curs == NULL)
		exit_err("no usable adr found");
	freeaddrinfo(res);
	goraw();
	pass_through(sock, STDOUT_FILENO);
}


static int serve_clientfd;
static void handle(int sig, siginfo_t *info, void *ctx) {
	close(serve_clientfd);
}
static void setup_sigchild() {
	struct sigaction sigact;
	sigact.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_restorer = NULL;
	sigact.sa_sigaction = handle;
	if (sigaction(SIGCHLD, &sigact, NULL) == -1)
		exit_err("sigaction"); 
}

static void serve(int fd, int sock) {
	int master, slave, nready;
	char *snam;
	uid_t suid;

	if ((master = posix_openpt(O_RDWR)) == -1)
		exit_err("posix_openpt");
	if (unlockpt(master) == -1)
		exit_err("unlockpt");
	if (grantpt(master) == -1)
		exit_err("granpt");
	if ((snam = ptsname(master)) == NULL)
		exit_err("ptsname");

	switch (fork()) {
	case -1:
		perror("fork");
		break;
	case 0:
		close(sock);
		setsid();
		if ((slave = open(snam, O_RDWR)) == -1)
			exit_err("open slave");
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		dup2(slave, STDIN_FILENO);
		seteuid(0);
		// Just doing seteuid isn't enough, and indeed, results in a very confusing error from login...
		setuid(0);
		if (getuid() != 0)
			exit_err("setuid fail. Please run this program as setuid");
		execl("/bin/login", "login", (char*)NULL);
		perror("execve");
	default:
		break;
	}
	close(slave);
	// Want to close the client when login exits.
	serve_clientfd = fd;
	setup_sigchild();
	pass_through(fd, master);
	close(fd);
	close(master);
}


static void server() {
	int sock, client;
	struct sockaddr_storage addr;
	socklen_t addrlen;
	char host[100], serv[100], c;
	// Rely on an ephemeral port automatically being selected to avoid
	// some socket setup code.


	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	if (listen(sock, 10) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	addrlen = sizeof (addr);
	if (getsockname(sock, (struct sockaddr*)&addr, &addrlen) == -1) {
		perror("getsockname");
		exit(EXIT_FAILURE);
	}
	if (getnameinfo((struct sockaddr*)&addr, addrlen, host, 100, serv,
				100, NI_NUMERICHOST | NI_NUMERICSERV) == -1) {
		perror("getnameinfo");
		exit(EXIT_FAILURE);
	}

	printf("Server starting on %s:%s\n", host, serv);
	openlog("rsh", LOG_PERROR, LOG_DAEMON);

	while ((client = accept(sock, NULL, NULL)) != -1) {
		puts("Accepting client");
		switch (fork()) {
		case -1:
			exit_err("fork");
		case 0:
			serve(client, sock);
			exit(EXIT_SUCCESS);
		default:
			close(client);
			break;
		}
	}
	close(sock);
	closelog();
}

int main(int argc, char **argv) {
	seteuid(getuid());
	if (argc > 1 && strcmp("server", argv[1]) == 0) {
		server();
	} else if (argc > 3 && strcmp("client", argv[1]) == 0) {
		// Also sets saved uid as per manpage
		setreuid(getuid(), getuid());
		client(argv[2], argv[3], argv[4]);
	} else {
		setreuid(getuid(), getuid());
		puts("Usage: ./rsh (server|client <host> <port>)");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
