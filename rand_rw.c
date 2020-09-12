/*
Create n files with a random sequence of names of x0-xn.
Then delete them sequentally. If SEQUENTIAL is defined, then
create and delete in the same order.
*/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

struct link {
	int value;
	struct link *prev;
	struct link *next;
};

struct list {
	struct link *l;
	int sz;
	int cursor;
};

void error(char *reason) {
	puts(reason);
	exit(EXIT_FAILURE);
}

void genlist(int sz, struct list *list) {
	struct link *head = malloc(sizeof (struct link));
	head->prev = NULL;
	head->next = NULL;
	head->value = 0;
	struct link *clink = head;
	for (int i = 1; i < sz; i++) {
		struct link *link = malloc(sizeof (struct link));
		clink->next = link;
		link->prev = clink;
		link->next = NULL;
		link->value = i;
		clink = link;
	}
	list->l = head;
	list->sz = sz;
	list->cursor = 0;
}

int remove_random(struct list *list) {
	int idx, offset, value;
	struct link *l;
	
	if (list->sz <= 0) {
		return -1;
	}
	
	idx = (int)((double)list->sz * ((double)random() / (double)RAND_MAX));
	offset = idx - list->cursor;
	if (offset > 0) {
		while (offset > 0) {
			list->l = list->l->next;
			list->cursor++;
			offset--;
		}
	} else {
		while (offset < 0) {
			list->l = list->l->prev;
			list->cursor--;
			offset++;
		}
	}
	
	l = list->l;
	value = l->value;
	
	if (l->next != NULL) {
		list->l = l->next;
	} else {
		list->l = l->prev;
		list->cursor--;
	}
	
	if (l->next != NULL) {
		l->next->prev = l->prev;
	}
	if (l->prev != NULL) {
		l->prev->next = l->next;
	}
	list->sz--;
	
	free(l);
	
	return value;
}

void get_filename(int i, char *dst) {
	sprintf(dst, "x%06d", i);
}

void create_file(int i) {
	int fd;
	char filename[7];
	
	get_filename(i, filename);
	if ((fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) == -1)
		error("open1\n");
	if (write(fd, "a", 1) != 1)
		error("write1\n");
	if (close(fd) == -1)
		error("close1\n");
}

void delete_file(int i) {
	char filename[7];
	
	get_filename(i, filename);
	if (remove(filename) == -1)
		error("remove1\n");
}


int main(int argc, char **argv) {
	int fd, sz, num;
	struct list list;
	
	srandom((unsigned int)time(NULL));
	
	if (argc < 2)
		error("usage: rand_rw <number>\n");
	
	sz = (int)strtol(argv[1], NULL, 10);
	genlist(sz, &list);
	#ifdef SEQUENTIAL
	for (int i = 0; i < sz; i++) {
		create_file(i);
	}
	#else
	while ((num = remove_random(&list)) > -1) {
		create_file(num);
	}
	#endif
	
	for (int i = 0; i < sz; i++) {
		delete_file(i);
	}
	
	exit(EXIT_SUCCESS);
}
