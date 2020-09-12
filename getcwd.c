/*
A reimplementation of getcwd using readdir and comparing inode numbers.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define ROOT_INODE 2

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

char *getcwd(char *buf, size_t size) {
	DIR *dirp;
	struct dirent *dirent;
	ino_t inode;
	int len;
	char *bufstart;
	int finished;
	
	bufstart = buf;
	buf[size - 1] = '\0';
	finished = 0;
	// Start at the end of the string, and work backwards.
	buf += size - 2;
	while (!finished) {
		if ((dirp = opendir(".")) == NULL)
			error("opendir1");
		errno = 0;
		while ((dirent = readdir(dirp)) != NULL) {
			if (strncmp(dirent->d_name, ".", 2) == 0) {
				inode = dirent->d_ino;
				if (closedir(dirp) == -1)
					error("closedir1");
				if (inode == ROOT_INODE) {
					finished = 1;
					break;
				}
				errno = 0;
				if ((dirp = opendir("..")) == NULL)
					error("opendir2");
				errno = 0;
				while ((dirent = readdir(dirp)) != NULL) {
					if (dirent->d_ino == inode) {
						len = strlen(dirent->d_name);
						buf -= len;
						if (buf < bufstart) {
							finished = 1;
							break;
						}
						strncpy(buf, dirent->d_name, len);
						buf -= 1;
						if (buf < bufstart) {
							finished = 1;
							break;
						}
						buf[0] = '/';
					}
				}
				if (closedir(dirp) == -1)
					error("closedir2");
				if (errno != 0)
					error("readdir1");
				break;	
			}
		}
		if (errno != 0)
			error("readdir2");
		chdir("..");
		if (finished)
			break;
	}
	// Shift the data to the start of the buffer
	len = size - (buf - bufstart);
	memmove(bufstart, buf, size);
	return bufstart;
}

int main(int argc, char **argv) {
	char buf[PATH_MAX], *ret;
	ret = getcwd(buf, PATH_MAX);
	puts(ret);
	exit(EXIT_SUCCESS);
}
