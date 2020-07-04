/*
reproduce chmod a+rX 
enable read persmissions on dirs and files
enable execute permission for directories
enable execute permission on files with x on any other category 
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

void update_permissions(char *file) {
	struct stat fstat;
	mode_t mode, allx;
	
	if (stat(file, &fstat) == -1) {
		error("stat\n");
	}
	mode = fstat.st_mode;
	mode |= (S_IRUSR | S_IRGRP | S_IROTH);
	
	allx = S_IXUSR | S_IXGRP | S_IXOTH;
	if (S_ISDIR(fstat.st_mode) || fstat.st_mode & allx) {
		mode |= allx;
	}
	
	if (chmod(file, mode) == -1) {
		error("chmod\n");
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		error("usage: chmodarx <file> ...\n");
	}
	
	for (int i = 1; i < argc; i++) {
		update_permissions(argv[i]);
	}
	exit(EXIT_SUCCESS);
}
