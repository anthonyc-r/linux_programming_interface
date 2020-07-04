#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * A very basic malloc implementation.
 * Does not do block merging, or decrease the program break like
 * actual malloc, but will reuse freed memory if there's a block that is the right size.
 */


extern char etext, edata, end;

void describe_break() {
    char *cbrk = sbrk(0);
    printf("======================\n");
    printf("ETEXT:\t%10p \n", &etext);
    printf("EDATA:\t%10p \n", &edata);
    printf("END:\t%10p \n", &end);
    printf("BRK:\t%10p \n", cbrk);
    printf("HSZ:\t%d \n", cbrk - &end);
    printf("======================\n");
}

void error(char *err) {
    printf(err);
    exit(EXIT_FAILURE);
}

static int needinit = 1;
static struct fblock *hbase;

// Not sure if packed is actually needed here, 
// but we rely on adjusting the pointer back from .data
// and assume this is .sz.
struct fblock {
    int sz;
    struct fblock *prev;
    struct fblock *next;
}  __attribute__((packed));

struct ublock {
    int sz;
    char data;
} __attribute__((packed));

int initmalloc() {
    int req;
    
    if((hbase = sbrk(0)) == (void*)-1)
        return -1;
    
    req = sizeof (struct fblock);
    sbrk(req);
    
    hbase->sz = 0;
    hbase->prev = NULL;
    hbase->next = NULL;
    
    return 0;
}

struct ublock* new_ublock(int reqsz) {
    struct ublock *block;
 
    reqsz = sizeof (struct fblock) - sizeof (int) > reqsz ? sizeof (struct fblock) - sizeof (int) : reqsz;
    if ((block = sbrk(reqsz)) == (void*)-1)
        return NULL;
    block->sz = reqsz ;
    return block;
}

struct fblock* next_good_fblock(int reqsz) {
    struct fblock *cblock;
    
    if (reqsz == 0)
        return NULL;
    
    cblock = hbase;
    while (1) {
        if (cblock->sz >= reqsz) {
            return cblock;
        } else if (cblock->next == NULL) {
            return NULL;
        }
        cblock = cblock->next;
    }
}

void* my_malloc(int n) {
    char *current_brk;
    struct fblock *goodblock;
    struct ublock *newblock;
    
    if((current_brk = sbrk(0)) == (void*)-1)
        return NULL;
    
    // Initilize the heap for use with mymalloc
    if (needinit) {
        if (initmalloc() == -1)
            return NULL;
        else
            needinit = 0;
    }
    
    goodblock = next_good_fblock(n);
    if (goodblock != NULL) {
		goodblock->prev->next = goodblock->next;
        return (void*)&(goodblock->prev);
    } else {
        newblock = new_ublock(n);
        if (newblock == NULL)
            return NULL;
        return (void*)&(newblock->data);
    }
}

void my_free(void *mem) {
    struct fblock *freed, *second;
    
    freed = (struct fblock*)(mem - sizeof (int));
	second = hbase->next;
	hbase->next = freed;
	freed->next = second;
	freed->prev = hbase;
	if (second != NULL) {
		second->prev = freed;
	}
}

int main(int argc, char **argv) {
    char *p;
    int i;
    

    describe_break();
	printf("mallocing!!\n");
    p = my_malloc(20);
	printf("requested with size 20 -> p:\t%10p \n", p);
	memcpy(p, "lmao!", 5);
	describe_break();
	printf("freeing & mallocing again!!!\n");
	my_free(p);
	
	p = my_malloc(6);
	printf("requested with size 6 -> p:\t%10p \n", p);
	memcpy(p, "lmao!", 6);
	printf("string: %s\n", p);
	describe_break();
	
	my_free(p);
	p = my_malloc(18);
	printf("requested with size 18 -> p:\t%10p \n", p);
	
    exit(EXIT_SUCCESS);
}
