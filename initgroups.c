 /**
  * Implement initgroups()
  */
 
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

int myinitgroups(const char *user, gid_t group) {
	int ngrps;
	uid_t uid;
	gid_t gids[NGROUPS_MAX + 1];
	struct pwd *pwd;
	struct group *grp;
	char *name;

	ngrps = 0;
	while ((grp = getgrent()) != NULL) {
		for (int i = 0; (name = grp->gr_mem[i]); i++) {
			if (strcmp(name, user) == 0) {
				gids[ngrps++] = grp->gr_gid;
			}
		}
	}
	endgrent();
	gids[ngrps] = group;
	
	return setgroups(ngrps, gids);
}

int main(int argc, char **argv) {
	if (myinitgroups("meguca", 0) == -1) {
		printf("Failed to init groups, is this process privilaged?\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
