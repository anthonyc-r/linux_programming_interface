#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/xattr.h>
#include <string.h>

int main(int argc, char **argv) {
	char opt, *name, *value, *file;
	
	while ((opt = getopt(argc, argv, "n:v:")) != -1) { 
		switch (opt) {
			case 'n':
				name = malloc(strlen(optarg) + 5);
				sprintf(name, "user.%s", optarg);
				break;
			case 'v':
				value = optarg;
				break;
		}
	}
	if (optind >= argc) {
		printf("Usage ./setfattr -n name -v value file\n");
		exit(EXIT_FAILURE);
	}	
	file = argv[optind];
	printf("value: %s\n", value);
	if (setxattr(file, name, value, strlen(value), 0) == -1) {
		printf("setxattr\n");
		exit(EXIT_FAILURE);
	}
	
	exit(EXIT_SUCCESS);
}
