/**
* Long running program that slowly generates output.
* For testing unbuffer.c
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define GREPPED_FOR "jdofisskd"

int main(int argc, char **argv) {
    int i;
    puts(GREPPED_FOR);
    for (i = 0; i < 10; i++) {
        puts("junk");
        sleep(1);
    }
    puts("");
    exit(EXIT_SUCCESS);
}