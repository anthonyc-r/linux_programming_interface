/**
 * Demonstrate that it's possible to call listen on a TCP inet socket
 * without first bindin it, and the result is a socket assigned to an
 * ephemeral port number
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <fcntl.h>

int main(int argc, char **argv) {
	int sock, ephd;
	struct sockaddr_storage addr;
	char host[100], serv[100];
	socklen_t addrlen;

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
	if (getnameinfo((struct sockaddr*)&addr, addrlen, host, 100, serv, 100, 
				NI_NUMERICHOST | NI_NUMERICSERV) == -1) {
		perror("getnameinfo");
		exit(EXIT_FAILURE);
	}
	puts("Ephemeral port range defined in /proc/sys/net/ipv4/ip_local_port_range");
	ephd = open("/proc/sys/net/ipv4/ip_local_port_range", O_RDONLY);
	sendfile(STDOUT_FILENO, ephd, NULL, 100);
	puts("Host:");
	puts(host);
	puts("Serv:");
	puts(serv);
	
	exit(EXIT_SUCCESS);
}
