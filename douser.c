/*
* A simplified 'sudo' implementation, requires the password for -u user/root
* ./douser [-u user] bin args...
* Resulting executable should be set up as set-user-id root
*
*/
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void exit_usage() {
	setuid(getuid());
	puts("usage: ./douser [-u username] cmd args...");
	exit(EXIT_FAILURE);	
}

static void exit_error() {
	int save;
	
	save = errno;
	setuid(getuid());
	errno = save;
	printf("douser: %s\n", strerror(errno));
	exit(errno);
}

static inline void drop_priv() {
	seteuid(getuid());
	if (geteuid() != getuid())
		exit(EXIT_FAILURE);
}

static inline void raise_priv() {
	if (seteuid(0) == -1)
		exit_error();
}

static inline void clearstr(char *dest) {
	memset(dest, 0, strlen(dest));
}

int main(int argc, char **argv) {
	int encr_len, ch;
	char *pwd, *encr, *unam;
	struct passwd* passwd;
	struct spwd *spwd;

	drop_priv();
	// Get the target user.
	unam = NULL;
	while ((ch = getopt(argc, argv, "u:")) != -1) {
		switch (ch) {
		case 'u':
			unam = optarg;
			break;
		default:
			exit_usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		exit_usage();	
	if (unam == NULL)
		unam = "root";

	// Request their password...
	if ((passwd = getpwnam(unam)) == NULL)
		exit_error();
	if ((pwd = getpass("password: ")) == NULL)
		exit_error();

/* BEGIN PRIV */
	// Get their encrypted password to compare with
	raise_priv();
	if ((spwd = getspnam(passwd->pw_name)) == NULL)
		exit_error();
	drop_priv();
/* END PRIV */
	
	// Encrypt input and check against shadow
	encr_len = strlen(spwd->sp_pwdp);
	if ((encr = crypt(pwd, spwd->sp_pwdp)) == NULL)
		exit_error();
	// Overwrite sensitive memory when no longer needed
	clearstr(pwd);
	if (strncmp(encr, spwd->sp_pwdp, encr_len + 1) != 0) {
		clearstr(spwd->sp_pwdp);
		clearstr(encr);
		puts("Bad password.");
		exit(EXIT_FAILURE);
	}
	clearstr(spwd->sp_pwdp);
	clearstr(encr);

	// Set groups and uid and exec.
/* BEGIN PRIV */
	raise_priv();
	if (initgroups(unam, passwd->pw_uid) == -1)
		exit_error();
	if (setgid(passwd->pw_gid) == -1)
		exit_error();
/* BECOME -u & exec*/
	if (setuid(passwd->pw_uid) == -1)
		exit_error();
	execvp(argv[0], argv);
	exit_error();
}