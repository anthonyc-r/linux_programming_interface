/*
* Verify that SIGSEV occurs when accessing unmapped memory, and SIGBUS occurs
* when accessing mapped memory that lies outside of the mapped file.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>

#define TMP_FILE "/tmp/mmap_sig_file.tmp"

static sigjmp_buf jenv;

static void exit_err(char *reason) {
	perror(reason);
	exit(EXIT_FAILURE);
}

static void handle(int sig) {
	// Not async signal safe...
	printf("Signal Received! %s\n", strsignal(sig));
	siglongjmp(jenv, 1);
}

int main(int argc, char **argv) {
	int fd, msz, fsz;
	char *mem, c;
	struct sigaction sigact;

	fsz = sysconf(_SC_PAGESIZE);
	msz = 2 * fsz;

	sigact.sa_handler = handle;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(SIGSEGV, &sigact, NULL) | sigaction(SIGBUS, &sigact, NULL) != 0)
		exit_err("sigaction");

	unlink(TMP_FILE);
	if ((fd = open(TMP_FILE, O_CREAT | O_RDWR, S_IRWXU)) == -1)
		exit_err("open");
	if (ftruncate(fd, fsz) == -1)
		exit_err("ftruncate");
	if ((mem = mmap(NULL, msz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == NULL)
		exit_err("mmap");

	puts("Accessing valid memory...");
	mem[0] = 'a';
	puts("Accessing unmapped memory... SIGSEGV expected");
	if (sigsetjmp(jenv, 1) == 0)
		mem[msz + 1] = 'b';
	puts("Accessing mapped memory outside of mapped file... SIGBUS exepcted.");
	if (sigsetjmp(jenv, 1) == 0)
		mem[fsz + 1] = 'c';

	exit(EXIT_SUCCESS);
}
