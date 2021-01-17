/*
* Loads both libwords and libsort, however, when we unload libsort - it will still be
* in memory, as it is also used by libwords. Tested on OpenBSD.
*/

#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

#include "sort.h"

static char *words[] = {
	"elephant",
	"rhino",
	"catalogue",
	"a",
	"the"
};

static void pword(char *str) {
	puts(str);
}

static void pinfo(Dl_info *info) {
	puts("************ dladdr info! *******************");
	printf("dli_fname:\t%s\n", info->dli_fname);
	printf("dli_fbase:\t%p\n", info->dli_fbase);
	printf("dli_sname:\t%s\n", info->dli_sname);
	printf("dli_saddr:\t%p\n", info->dli_saddr);
	puts("*********************************************");
}

int main(int argc, char **argv) {
	Dl_info info;
	void *libsort, *libwords;
	void (*listwords)(char **, int, void (*)(char *));
	void (*sort)(struct sortee *, int);

	puts("Opening libsort");
	libsort = dlopen("libsort.so", RTLD_NOW);
	if (libsort == NULL) {
		puts("libsort not found");
		exit(EXIT_FAILURE);
	}
	*(void **)(&sort) = dlsym(libsort, "sort");
	if (sort == NULL) {
		puts("(libwords) sym sort not found");
		exit(EXIT_FAILURE);
	}

	puts("Opening libwords");
	libwords = dlopen("libwords.so", RTLD_NOW);
	if (libsort == NULL) {
		puts("libwords not found");
		exit(EXIT_FAILURE);
	}
	*(void **)(&listwords) = dlsym(libwords, "listwords");
	if (listwords == NULL) {
		puts("sym listwords not found");
		exit(EXIT_FAILURE);
	}

	puts("***** calling libwords *****");
	listwords(words, 5, pword);
	puts("****************************");

	puts("Closing libsort");
	if (dlclose(libsort) != 0) {
		puts("Failed to close libsort");
		exit(EXIT_FAILURE);
	}

	puts("Checking if libsort is still loaded...");
	(void)dlerror();
	if (dladdr(sort, &info) == 0) {
		printf("libsort is not loaded! (%s)\n", dlerror());
		exit(EXIT_FAILURE);
	} else {
		puts("libsort is loaded!");
	}

	pinfo(&info);

	puts("Closing libwords");
	if (dlclose(libwords) != 0) {
		puts("Failed to close libwords");
		exit(EXIT_FAILURE);
	}

	puts("Checking if libsort is still loaded...");
	if (dladdr(sort, &info) == 0) {
		puts("libsort is not loaded!");
	} else {
		puts("libsort is unexpectedly still loaded for some reason...");
		exit(EXIT_FAILURE);
	}
	
	
	exit(EXIT_SUCCESS);
}