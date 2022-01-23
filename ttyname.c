#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>

static char _myttyname[PATH_MAX];

#define PATH_VIRT "/dev"
#define PATH_PSUDO "/dev/pts"

static char *my_ttyname_path(int fd, char *path) {
	struct dirent *ent;
	struct stat sb;
	DIR *dir;
	char *res;
	ino_t ino;
	dev_t dev;

	if (fstat(fd, &sb) == -1)
		return NULL;
	ino = sb.st_ino;
	dev = sb.st_dev;

	if ((dir = opendir(path)) == NULL)
		return NULL;
	if (chdir(path) == -1)
		return NULL;
	while ((ent = readdir(dir)) != NULL) {
		if (stat(ent->d_name, &sb) == -1)
			continue;
		if (sb.st_ino == ino && sb.st_dev == dev)
			return realpath(ent->d_name, _myttyname);
		else
			continue;
	}
	
	return res;
}

char *my_ttyname(int fd) {
	char *res;

	if ((res = my_ttyname_path(fd, PATH_VIRT)) != NULL)
		return res;
	if ((res = my_ttyname_path(fd, PATH_PSUDO)) != NULL)
		return res;
}

int main(int argc, char **argv) {
	char *name;

	name = my_ttyname(STDIN_FILENO);
	if (name != NULL) {
		puts(name);
	} else {
		puts("Couldn't find ttyname");
	}
	exit(EXIT_SUCCESS);
}
