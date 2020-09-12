/**
display acl entries applying to the specified user or group
getacl -u 1000 file
*/

// Non-portable - acl_get_perm

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/acl.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <errno.h>
#include <acl/libacl.h>
#include <sys/stat.h>

const char *usage = "Usage: getacl (-u user | -g group) filename\n";

void err(const char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

void pperms(acl_permset_t perms) {
	char permstr[4];
	
	errno = 0;
	permstr[0] = acl_get_perm(perms, ACL_READ) == 1 ? 'r' : '-';
	permstr[1] = acl_get_perm(perms, ACL_WRITE) == 1 ? 'w' : '-';
	permstr[2] = acl_get_perm(perms, ACL_EXECUTE) == 1 ? 'x' : '-';
	permstr[3] = '\0';
	if (errno != 0)
		err("acl_get_perm\n");
	puts(permstr);
}
void pperms_masked(acl_permset_t perms, acl_permset_t mask) {
	char permstr[4];
	
	errno = 0;
	permstr[0] = acl_get_perm(perms, ACL_READ) && 
		acl_get_perm(mask, ACL_READ) == 1 ? 'r' : '-';
	permstr[1] = acl_get_perm(perms, ACL_WRITE) && 
		acl_get_perm(mask, ACL_WRITE) == 1 ? 'w' : '-';
	permstr[2] = acl_get_perm(perms, ACL_EXECUTE) && 
		acl_get_perm(mask, ACL_EXECUTE) == 1 ? 'x' : '-';
	permstr[3] = '\0';
	if (errno != 0)
		err("acl_get_perm\n");
	puts(permstr);
}

// if user == 0 assume we're looking for group entries, else user entries
void getacl(char *identifier, char *file, int user) {
	char *ep;
	long id, tagid;
	struct passwd *pwd;
	struct group *grp;
	struct stat fstat;
	acl_t acl;
	int acli, status;
	acl_entry_t aclent; 
	acl_type_t acltag;
	acl_permset_t perms, maskperms;

	// Get the numeric ID
	id = strtol(identifier, &ep, 10);
	if (*ep != '\0') {
		if (user) {
			if ((pwd = getpwnam(identifier)) == NULL)
				err("invalid username or id\n");
			id = pwd->pw_uid;
		} else {
			if ((grp = getgrnam(identifier)) == NULL)
				err("invalid group name or gid\n");
			id = grp->gr_gid;
		}
	}
	
	// Get owner or group of file.
	if (stat(file, &fstat) == -1)
		err("stat\n");
	
	if ((acl = acl_get_file(file, ACL_TYPE_ACCESS)) == NULL)
		err("acl_get_file\n");
	for (acli = ACL_FIRST_ENTRY; ; acli = ACL_NEXT_ENTRY) {
		if ((status = acl_get_entry(acl, acli, &aclent)) != 1)
			break;
		if (acl_get_tag_type(aclent, &acltag) == -1)
			err("acl_get_tag_type\n");
		if (acltag == ACL_MASK) {
			acl_get_permset(aclent, &maskperms);
		}
	}
	printf("--- acl entries applying to %s: <%s> ---\n", 
		user ? "user" : "group", identifier);
	for (acli = ACL_FIRST_ENTRY; ; acli = ACL_NEXT_ENTRY) {
		if ((status = acl_get_entry(acl, acli, &aclent)) != 1)
			break;
		if (acl_get_tag_type(aclent, &acltag) == -1)
			err("acl_get_tag_type\n");
		switch (acltag) {
			case ACL_USER_OBJ:
				if (user && fstat.st_uid == id) {
					acl_get_permset(aclent, &perms);
					printf("* ACL_USER_OBJ *\n");
					printf("permissions:\t");
					pperms(perms);
				}
				break;
			case ACL_USER:
				if (user && *(uid_t*)acl_get_qualifier(aclent) == id) {
					acl_get_permset(aclent, &perms);
					printf("* ACL_USER *\n");
					printf("permissions:\t");
					pperms(perms);
					printf("mask:\t\t");
					pperms(maskperms);
					printf("effective:\t");
					pperms_masked(perms, maskperms);
					return;
				}
				break;
			case ACL_GROUP_OBJ:
				if (!user && fstat.st_gid == id) {
					acl_get_permset(aclent, &perms);
					printf("* ACL_GROUP_OBJ *\n");
					printf("permissions:\t");
					pperms(perms);
				}
				break;
			case ACL_GROUP:
				if (!user && *(gid_t*)acl_get_qualifier(aclent) == id) {
					acl_get_permset(aclent, &perms);
					printf("* ACL_GROUP *\n");
					printf("permissions:\t");
					pperms(perms);
					printf("mask:\t\t");
					pperms(maskperms);
					printf("effective:\t");
					pperms_masked(perms, maskperms);
					return;
				}
				
		}
	}
	if (status == -1)
		err("acl_get_entry\n");
}

int main(int argc, char **argv) {
	char c, *user, *group, *file;
	
	user = NULL;
	group = NULL;
	while((c = getopt(argc, argv, "u:g:")) != -1) {
		switch (c) {
			case 'u':
				user = optarg;
				break;
			case 'g':
				group = optarg;
				break;
			case '?':
				err(usage);
				break;
		}
	}
	if (optind >= argc)
		err(usage);
	file = argv[optind];
	
	if (user != NULL)
		getacl(user, file, 1);
	else if (group != NULL)
		getacl(group, file, 0);
	else
		err(usage);

	exit(EXIT_SUCCESS);
}
