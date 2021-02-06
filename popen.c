/**
* An implementation of popen and pclose
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>


// For the sake of simplicity
#define POPEN_MAX 100
struct pinfo {
	int active;
	pid_t pid;
};
static struct pinfo pinfo[POPEN_MAX];

FILE *my_popen(const char *command, const char *type) {
	int fds[2], write, saved_errno, i;
	pid_t child;

	if (strncmp(type, "w", 2) == 0 || strncmp(type, "we", 3) == 0) {
		write = 1;
	} else if (strncmp(type, "r", 2) == 0 || strncmp(type, "re", 3) == 0) {
		write = 0;		
	} else {
		errno = EINVAL;
		return NULL;
	}

	// Write end = 1, read end = 0.
	if (pipe(fds) == -1)
		return NULL;
	if (fds[1] >= POPEN_MAX || fds[0] >= POPEN_MAX) {
		errno = ERANGE;
		return NULL;
	}
	// Set CLOEXEC on the fd that the parent process keeps.
	if(type[1] == 'e' && fcntl(write ? fds[1] : fds[0], F_SETFD, FD_CLOEXEC) == -1) {
		saved_errno = errno;
		close(fds[0]);
		close(fds[1]);
		errno = saved_errno;
		return NULL;
	}

	switch ((child = fork())) {
	case -1:
		return NULL;
	case 0:
		// SUSv3 specifies that all open fds from earlier popens be closed.
		for (i = 0; i < POPEN_MAX; i++) {
			if (pinfo[i].active) {
				close(i);
			}
		}
		if (write) {
			close(fds[1]);
			if (fds[0] != STDIN_FILENO) {
				dup2(fds[0], STDIN_FILENO);
				close(fds[0]);
			}
		} else {
			if (fds[1] != STDOUT_FILENO) {
				dup2(fds[1], STDOUT_FILENO);
				close(fds[1]);
			}
			close(fds[0]);
		}
		execl("/bin/sh", "/bin/sh", "-c", command, (char*)NULL);
		_exit(EXIT_FAILURE);
	default:
		break;
	}
	
	if (write) {
		close(fds[0]);
		pinfo[fds[1]].active = 1;
		pinfo[fds[1]].pid = child;
		return fdopen(fds[1], "w");
	} else {
		close(fds[1]);
		pinfo[fds[0]].active = 1;
		pinfo[fds[0]].pid = child;
		return fdopen(fds[0], "r");
	}
}

int my_pclose(FILE *stream) {
	int fd, status;
	if ((fd = fileno(stream)) == -1)
		return -1;
	if (fclose(stream) == -1)
		return -1;
	waitpid(pinfo[fd].pid, &status, 0);
	pinfo[fd].active = 0;
	return status;
}

// Test
int main(int argc, char **argv) {
	char output[100];

	FILE *f = my_popen("sleep 2 && ls", "r");
	FILE *shell = my_popen("sh", "w");
	if (f == NULL) {
		printf("my_popen: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("Result:\n");
	while (fgets(output, 100, f)) {
		printf("\t%s", output);
	}
	my_pclose(f);
	fwrite("sleep 2 && echo Hello World", 1, 28, shell);
	my_pclose(shell);
	
	char query[50];
	printf("Search for?... ");
	fgets(query, 50, stdin);
	char command[100];
	snprintf(command, 100, "ls %s\n", query);
	f = my_popen(command, "r");
	while (fgets(output, 100, f)) {
		printf("\t%s", output);
	}
	exit(EXIT_SUCCESS);
}
