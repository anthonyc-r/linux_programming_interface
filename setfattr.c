#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char **argv) {
	char opt, *name, *value, *file;
	
	while ((opt = getopt(argc, argv, "n:v:")) != -1) { 
		switch (opt) {
			case 'n':
				name = optarg;
				break;
			case 'v':
				value = optarg;
				break;
		}
	}
	file = argv[optind];
	
	printf("name: %s, value: %s file: %s\n", name, value, file);
	
	exit(EXIT_SUCCESS);
}
