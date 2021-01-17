/*
* Provides a function to print the provided words in order of length. Makes use of libsort
* and will compile to libwords for use in the main program.
*/
#include <dlfcn.h>
#include <stdlib.h>

#include "sort.h"

static void *libsort;

// Note that if we don't properly unload libsort when WE are unloaded, libsort doesn't get 
// libsort stays loaded for the duration of execution!
void __attribute__ ((destructor)) libwords_unload() {
	if (libsort) {
		dlclose(libsort);
	}
}

// Intentionall left out static to test out version scripts (libwords.map)
int mystrlen(char *str) {
	int i;
	
	for (i = 0; str[i] != '\0'; i++)
		;
	return i;
}	

void listwords(char** words, int n, void (*puts)(char *)) {
	void (*sort)(struct sortee *, int);
	struct sortee *tosort;
	int i;

	libsort = dlopen("libsort.so", RTLD_NOW);
	if (libsort == NULL) {
		puts("(libwords) libsort not found");
		exit(EXIT_FAILURE);
	}
	*(void **)(&sort) = dlsym(libsort, "sort");
	if (sort == NULL) {
		puts("(libwords) sym sort not found");
		exit(EXIT_FAILURE);
	}

	tosort = malloc(n * sizeof (struct sortee));
	for (i = 0; i < n; i++) {
		tosort[i].sortby = mystrlen(words[i]);
		tosort[i].data = words[i];
	}
	sort(tosort, n);
	for (i = 0; i < n; i++) {
		puts((char*)tosort[i].data);
	}
}