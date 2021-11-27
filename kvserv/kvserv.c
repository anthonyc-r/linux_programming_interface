/*
 * A key value storage server
 * Keys can be inserted in the format 'key\tvalue'
 * Or fetched in the format 'key'
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define BUFSZ 100
#define SEP ':'

static int sock;

struct kvinput {
	int new;
	char key[BUFSZ];
	char value[BUFSZ];
};

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static int kvinput_parse(char *input, struct kvinput *kv) {
	char *sep, *kend;
	size_t len, klen, vlen;

	len = strlen(input);
	kend = input + len - 1;
	kv->new = 0;
	if ((sep = strrchr(input, '\n')) != NULL)
		*sep = '\0';
	if ((sep = strrchr(input, '\r')) != NULL)
		*sep = '\0';
	if ((sep = strchr(input, SEP)) != NULL) {
		kv->new = 1;
		kend = sep;
		*sep = '\0';
	}
	strncpy(kv->key, input, BUFSZ);
	strncpy(kv->value, kend + 1, BUFSZ);
	return 0;
}

static void handler(int sig) {
	puts("Shutting down");
	if (sock > -1)
		close(sock);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	struct addrinfo hints, *addr_res, *addr;
	int fd;
	ssize_t nread;
	char buf[BUFSZ];
	struct kvinput kv;
	char *ret;

	if (argc < 2)
		exit_err("./kvserv <service>");

	sock = -1;
	signal(SIGINT, handler);
	// We're going to use get/setenv to store out key value pairs (lol)
	if (clearenv() != 0)
		exit_err("clearenv");
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;
	hints.ai_protocol = 0;

	if (getaddrinfo(NULL, argv[1], &hints, &addr_res) == -1)
		exit_err("getaddrinfo");
	for (addr = addr_res; addr != NULL; addr = addr->ai_next) {
		if ((sock = socket(addr->ai_family, addr->ai_socktype, addr-> ai_protocol)) == -1)
			continue;
		if (bind(sock, addr->ai_addr, addr->ai_addrlen) == -1)
			continue;
		if (listen(sock, 10) == -1)
			continue;
		break;
	}
	if (addr == NULL)
		exit_err("No usable addresses");

	for (;;) {
		if ((fd = accept(sock, NULL, NULL)) == -1)
			exit_err("accept");
		if ((nread = read(fd, buf, BUFSZ)) < 1) {
			puts("read fail, skip client");
			close(fd);
			continue;
		}
		if (kvinput_parse(buf, &kv) == -1) {
			puts("parse fail");
			close(fd);
			continue;
		}
		// Exposing envirnment variables, but for the sake of a quick 
		// key value storage solution...
		printf("key: '%s'\n", kv.key);
		if (kv.new) {
			setenv(kv.key, kv.value, 1);
			printf("value: '%s'\n", kv.value);
		} else if ((ret = getenv(kv.key)) != NULL) {
			nread = snprintf(buf, BUFSZ, "%s\n", ret);
			printf("value: %s", buf);
			write(fd, buf, nread);
		} else {
			puts("Key not found");
			write(fd, "NOENT\n", 7);
		}
		close(fd);
	}
	
	exit(EXIT_FAILURE);
}
