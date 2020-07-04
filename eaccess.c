#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

enum MY_EACCESS_MODE {
	MF_OK,
	MR_OK,
	MW_OK,
	MX_OK
};


int my_eaccess(char *pathname, int mode) {
	struct stat fstat;
	struct pwd *pwd;
	uid_t uid;
	gid_t gid;
	
	
	uid = geteuid();
	gid = getegid();
	if (stat(pathname, &fstat) == -1) {
		return -1;
	}
	if (mode == MF_OK) {
		return 0;
	}
	if (fstat.st_uid == uid) {
		switch (mode) {
			case MR_OK:
				return fstat.st_mode & S_IRUSR ? 0 : -1;
			case MW_OK:
				return fstat.st_mode & S_IWUSR ? 0 : - 1;
			case MX_OK:
				return fstat.st_mode & S_IXUSR ? 0 : -1;
		}
	} else if (fstat.st_gid == gid) {
		switch (mode) {
			case MR_OK:
				return fstat.st_mode & S_IRGRP ? 0 : -1;
			case MW_OK:
				return fstat.st_mode & S_IWGRP ? 0 : -1;
			case MX_OK:
				return fstat.st_mode & S_IXGRP ? 0 : -1;
		}
	} else {
		switch (mode) {
			case MR_OK:
				return fstat.st_mode & S_IROTH ? 0 : -1;
			case MW_OK:
				return fstat.st_mode & S_IWOTH ? 0 : -1;
			case MX_OK:
				return fstat.st_mode & S_IXOTH ? 0 : -1;
		}
	}	
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("eaccess <filename>\n");
		exit(EXIT_FAILURE);
	}

	if (my_eaccess(argv[1], MF_OK) == 0) {
		printf("file exists\n");
	}
	if (my_eaccess(argv[1], MR_OK) == 0) {
		printf("can read\n");
	}
	if (my_eaccess(argv[1], MW_OK) == 0) {
		printf("can write\n");
	}
	if (my_eaccess(argv[1], MX_OK) == 0) {
		printf("can execute\n");
	}

	exit(EXIT_SUCCESS);
}
