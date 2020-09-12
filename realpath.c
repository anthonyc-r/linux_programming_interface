/*
Implementation of realpath
take a path possibly containing symlinks, and possibly relative
return the absolute path not containing symlinks.

Using linked lists might be a bit overkill here lol.
BUT, it seemed to me to be the best way to manipulate paths here.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

struct ll {
	char *component;
	struct ll *next;
	struct ll *prev;
};

struct ll *ll_link(const char *component, int len) {
	struct ll *link = calloc(1, sizeof(struct ll));
	link->component = malloc(len + 1);
	strncpy(link->component, component, len);
	link->component[len + 1] = '\0';
	return link;
}

struct ll *ll_insert(struct ll *list, struct ll *item) {
	item->next = NULL;
	item->prev = list;
	if (list == NULL) {
		return item;
	} else {
		item->next = list->next;
		list->next = item;
		return item;
	}
}

struct ll *ll_head(struct ll *list) {
	while (list->prev != NULL)
		list = list->prev;
	return list;
}

struct ll *ll_tail(struct ll *list) {
	while (list->next != NULL)
		list = list->next;
	return list;
}

void ll_substitute(struct ll *link, struct ll *items) {
	struct ll *ihead, *itail, *lbefore, *lafter;
	ihead = ll_head(items);
	itail = ll_tail(items);
	
	lbefore = link->prev;
	lafter = link->next;
	
	if (lbefore != NULL) {
		lbefore->next = ihead;
	}
	ihead->prev = lbefore;
	if (lafter != NULL) {
		lafter->prev = itail;
	}
	itail->next = lafter;	
}

void ll_remove(struct ll *link) {
	link->prev->next = link->next;
	if (link->next != NULL) {
		link->next->prev = link->prev;
	}
}

struct ll *ll_prepend(struct ll *first, struct ll *second) {
	struct ll *ftail, *shead;
	
	if (second == NULL && first == NULL) {
		return NULL;
	} else if (second == NULL) {
		return first;
	} else if (first == NULL) {
		return second;
	}
	ftail = ll_tail(first);
	shead = ll_head(second);
	shead->prev = ftail;
	ftail->next = shead;
	return first;
}

void ll_destroy(struct ll *list) {
	struct ll *head, *cursor;
	
	head = ll_head(list);
	cursor = head;
	while (head != NULL) {
		head = head->next; 
		free(cursor->component);
		free(cursor);
		cursor = head;
	}
}

struct ll *ll_gen(const char *path) {
	struct ll *list;
	const char *start, *end;
	
	list = NULL;
	while (1) {
		while (*path == '/')
			path++;
		start = path;
		while (*path != '/' && *path != '\0')
			path++;
		end = path;
		if (*path == '\0' && start == end)
			break;
		list = ll_insert(list, ll_link(start, end - start));
	}
	return list;
}

void ll_test(struct ll *list) {
	struct ll *cursor;
	cursor = list;
	while (cursor != NULL) {
		puts(cursor->component);
		cursor = cursor->prev;
	}
}

void error(char *err) {
	puts(err);
	exit(EXIT_FAILURE);
}

// prints the path up to this component
void strpath(struct ll *tail, char *dst) {
	struct ll *lis_cursor;
	char *str_cursor;
	
	lis_cursor = ll_head(tail);
	str_cursor = dst;
	while (lis_cursor != NULL && lis_cursor->prev != tail) {
		str_cursor += sprintf(str_cursor, "/%s", lis_cursor->component);
		lis_cursor = lis_cursor->next;
	}
}

void resolve_symlinks(struct ll *list) {
	int symlen;
	char symbuff[PATH_MAX], pathbuff[PATH_MAX];
	struct ll *lis_cursor, *prev, *symlist;
	
	lis_cursor = ll_tail(list);
	while(lis_cursor != NULL) {
		prev = lis_cursor->prev;
		strpath(lis_cursor, pathbuff);
		if ((symlen = readlink(pathbuff, symbuff, PATH_MAX)) != -1) {
			symbuff[symlen] = '\0';
			symlist = ll_gen(symbuff);
			ll_substitute(lis_cursor, symlist);
		}
		lis_cursor = prev;
	}
}

void prune_dots(struct ll *list) {
	struct ll *cursor, *prev;
	
	cursor = ll_tail(list);
	while (cursor != NULL) {
		prev = cursor->prev;
		if (strcmp(cursor->component, ".") == 0) {
			puts("remove");
			ll_remove(cursor);
		} else if (strcmp(cursor->component, "..") == 0) {
			prev = prev->prev;
			ll_remove(cursor->prev);
			ll_remove(cursor);
		}
		cursor = prev;
	}
}


char *myrealpath(const char *path, char *resolved) {
	struct ll *list;
	struct stat sbuf;
	char buff[PATH_MAX];
	// check if file exists... (todo: check errno....)
	if (stat(path, &sbuf) == -1)
		error("file does not exist\n");
	
	if (path[0] != '/') {
		//prepend the cwd
		if (getcwd(buff, PATH_MAX) == NULL)
			error("getcwd\n");
		list = ll_prepend(ll_gen(buff), ll_gen(path));
	} else {
		list = ll_gen(path);
	}
	resolve_symlinks(list);
	prune_dots(list);
	strpath(ll_tail(list), buff);
	puts(buff);

	ll_destroy(list);
	return resolved;
}


int main(int argc, char **argv) {
	char output[PATH_MAX];
	if (argc < 2)
		error("usage: realpath *path*\n");
	if(myrealpath(argv[1], output) == NULL)
		error("myrealpath\n");
	puts(output);
	exit(EXIT_SUCCESS);
}
