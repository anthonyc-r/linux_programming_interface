 /**
  * This program lists the PID and command name of all programs being run by
  * the speicified user. It makes use of the /proc directory to do this.
  */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <ctype.h>

#define PID_PATH_MAX 100
#define SYM_MAX_SZ 100

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

int skipline(int fd) {
	int nread;
	char c;
	while ((nread = read(fd, &c, 1)) > 0) {
		if (c == '\n') {
			return 1;
		}
	}
	if (nread == -1)
		error("read\n");
	return 0;
}

int seeksym(int fd, char *sym) {
	int len, cseek, nread;
	char buf[SYM_MAX_SZ];

	len = strlen(sym);
	if (len > SYM_MAX_SZ)
		error("seeksym sym too long\n");
	if (lseek(fd, 0, SEEK_SET) == -1)
		error("lseek\n");
	cseek = 0;
	while ((nread = read(fd, buf, len)) == len) {
		if (strncmp(sym, buf, len) == 0) {
			if (lseek(fd, cseek, SEEK_SET) == -1)
				error("lseek\n");
			return 1;
		}
		if (!skipline(fd))
			break;
		if ((cseek = (int)lseek(fd, 0, SEEK_CUR)) == -1)
			error("lseek\n");
	}
	if (nread ==  -1)
		error("read\n");
	return 0;
}

int main(int argc, char **argv) {
	struct dirent *dirent = NULL;
	DIR *dirp = NULL;
	int pid, fd, data_s, data_e;
	char *endptr, fp[PID_PATH_MAX], buf[100];
	struct passwd *pwd;
	uid_t uid;
	
	if (argc < 2)
		error("Missing argument. listproc username.\n");
	
	if ((dirp = opendir("/proc")) == NULL)
		error("opendir\n");
	
	errno = 0;
	if ((pwd = getpwnam(argv[1])) == NULL) {
		if (errno != 0)
			error("getpwnam\n");
		else
			error("can't find user\n");
	}
	uid = pwd->pw_uid;
	errno = 0;
	
	printf("PID\tUser\tProcess\n");
	while ((dirent = readdir(dirp)) != NULL) {
		pid = (int)strtol(dirent->d_name, &endptr, 10);
		if (*endptr != '\0')
			continue;
		if (sprintf(fp, "/proc/%d/status", pid) < 0)
			error("sprintf\n");
		if ((fd = open(fp, O_RDONLY)) == -1)
			error("open\n");
		if (seeksym(fd, "Uid:") != 1)
			error("can't find Uid: entry in /status\n");
		data_e = -1;
		if ((data_s = lseek(fd, 5, SEEK_CUR)) == -1)
			error("lseek\n");
		while ((read(fd, buf, 1)) == 1) {
			if (!isalnum(buf[0])) {
				if((data_e = lseek(fd, 0, SEEK_CUR)) == -1)
					error("lseek\n");
				break;
			}
		}
		if (data_e == -1)
			error("error finding delimiter for data\n");
		if ((lseek(fd, data_s, SEEK_SET)) == -1)
			error("lseek\n");
		if (read(fd, buf, data_e - data_s) == -1)
			error("read\n");
		buf[data_e - data_s - 1] = '\0';
		if (uid != (uid_t)strtol(buf, NULL, 10))
			continue;

			
		if (seeksym(fd, "Name:") != 1)
			error("Couldn't find Name in /status\n");
		if ((data_s = lseek(fd, 6, SEEK_CUR)) == -1)
			error("lseek\n");
		while ((read(fd, buf, 1)) == 1) {
			if (!isalnum(buf[0])) {
				if((data_e = lseek(fd, 0, SEEK_CUR)) == -1)
					error("lseek\n");
				break;
			}
		}
		if (data_e == -1)
			error("error finding delimiter for data\n");
		if ((lseek(fd, data_s, SEEK_SET)) == -1)
			error("lseek\n");
		if (read(fd, buf, data_e - data_s) == -1)
			error("read\n");
		buf[data_e - data_s - 1] = '\0';
		printf("%d\t%s\t%s\n", pid, argv[1], buf);
		
		errno = 0;
	}
	if (errno > 0)
		error("readdir\n");
	
	exit(EXIT_SUCCESS);
}
