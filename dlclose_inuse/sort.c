/* 
* Provides a simple sorting function. Compiles to libsort, which be loaded by
* the main program, but also another library. It will then be unloaded by the main program
* to demonstrate that even after dlclose, it is not closed, as it's still in use.
*/
#include "sort.h"

void sort(struct sortee items[], int n) {
	int swapped, i;
	struct sortee tmp;

	do {
		swapped = 0;
		for (i = 0; i < (n - 1); i++) {
			if (items[i].sortby > items[i + 1].sortby) {
				tmp = items[i];
				items[i] = items[i + 1];
				items[i + 1] = tmp;
				swapped = 1;
			}
		}
	} while (swapped);
}