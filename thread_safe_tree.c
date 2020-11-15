/*
* A thread safe implementation of an unbalanced binary tree.
* With 1000 threads doing 100 additions:
* Time with single lock:
real    0m8.951s
user    0m8.818s
sys     0m3.343s
* Time with node lock:
real    0m7.565s
user    0m29.157s
sys     0m0.686s
*
* With 100 threads doing 1000 additions:
* Time with single lock:
real    0m3.802s
user    0m3.971s
sys     0m2.755s
* Time with node lock:
real    0m1.063s
user    0m3.913s
sys     0m0.192s
* Hmm. The difference between real and user time in the multi lock example shows how effective
* using fine grained locking is. However, it seems much more effective in the example with
* fewer threads doing more work, for some reason. I suppose it adds a lot of overhead in
* the example with 1000 threads.
*/
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#ifdef INTERACTIVE
#include <readline/readline.h>
#endif

#define LOCK_SINGLE(mtx) 
#define LOCK_MULTI(mtx) 
#define UNLOCK_SINGLE(mtx) 
#define UNLOCK_MULTI(mtx) 

#ifdef SINGLE_LOCK

#undef LOCK_SINGLE
#undef UNLOCK_SINGLE
#define LOCK_SINGLE(mtx) \
	pthread_mutex_lock(mtx);
#define UNLOCK_SINGLE(mtx) \
	pthread_mutex_unlock(mtx);
#else
#undef LOCK_MULTI
#undef UNLOCK_MULTI
#define LOCK_MULTI(mtx) \
	pthread_mutex_lock(mtx);

#define UNLOCK_MULTI(mtx) \
	pthread_mutex_unlock(mtx);

#endif



struct node {
	struct node *left;
	struct node *right;
	char *key;
	void *value;
#ifndef SINGLE_LOCK
	pthread_mutex_t *mtx;
#endif
};

struct tree {
	struct node *root;
	pthread_mutex_t *mtx;
};

static void add_node(struct tree *tree, struct node *node) {
	struct node **dest, *parent;
	
	dest = &tree->root;
	parent = NULL;
	while (*dest != NULL) {
		parent = *dest;
		if (strncmp(node->key, (*dest)->key, strlen(node->key) + 1) > 0) {
			dest = &(*dest)->right;
		} else {
			dest = &(*dest)->left;
		}
	}
	if (parent != NULL) {
		LOCK_MULTI(parent->mtx);
		*dest = node;
		UNLOCK_MULTI(parent->mtx);
	} else {
		LOCK_MULTI(tree->mtx);
		*dest = node;
		UNLOCK_MULTI(tree->mtx);
	}
}

void initialize(struct tree *tree) {
	memset(tree, 0, sizeof(struct tree));
	tree->root = NULL;
	// Default attributes.
	tree->mtx = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(tree->mtx, NULL);
}

void add(struct tree *tree, char *key, void *value) {
	struct node *new;

	new = calloc(1, sizeof (struct node));
	new->value = value;
	new->key = key;
#ifndef SINGLE_LOCK
	new->mtx = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(new->mtx, NULL);
#endif

	LOCK_SINGLE(tree->mtx);
	add_node(tree, new);
	UNLOCK_SINGLE(tree->mtx);
}

void delete(struct tree *tree, char *key) {
	struct node **dest, *deleted, *parent;
	int cmp;

	LOCK_SINGLE(tree->mtx);
	dest = &tree->root;
	while (*dest != NULL) {
		parent = *dest;
		cmp = strncmp(key, (*dest)->key, strlen(key) + 1);
		if (cmp == 0) {
			break;
		} else if (cmp > 0) {
			dest = &(*dest)->right;
		} else {
			dest = &(*dest)->left;
		}
	}
	LOCK_MULTI(parent->mtx);
	deleted = *dest;
	*dest = NULL; 
	UNLOCK_MULTI(parent->mtx);
	if (deleted != NULL) {
		if (deleted->left != NULL)
			add_node(tree, deleted->left);
		if (deleted->right != NULL)
			add_node(tree, deleted->right);
		free(deleted);
	}
	UNLOCK_SINGLE(tree->mtx);
}

// Read only, no need to lock?
bool lookup(struct tree *tree, char *key, void **value) {
	struct node *target;
	int cmp;

	target = tree->root;
	while (target != NULL) {
		cmp = strncmp(key, target->key, strlen(key) + 1);
		if (cmp == 0) {
			*value = target->value;
			return true;
		} else if (cmp > 0) {
			target = target->right;
		} else {
			target = target->left;
		}
	}
	return false;
}


// Quick test for the above implementation

static struct tree tree;

static void *run_thread(void *arg) {
	int i;
	char str[2];
	str[1] = '\0';

	for (i = 0, str[0] = 'a'; i < (int)arg; str[0]++, i++) {
		add(&tree, str, (void*)100);
	}
	return (void*)0;
}

int main(int argc, char **argv) {
	char *resp;
	int value, i;
	bool present;
	pthread_t thread;
	
	initialize(&tree);
#ifdef INTERACTIVE
	while (1) {
		resp = readline("a. Add; b. Del; c. Check; e. Exit\n");
		switch (*resp) {
		case 'a':
			resp = readline("value: ");
			value = (int)strtol(resp, NULL, 10);
			resp = readline("key: ");
			add(&tree, resp, (void*)value);
			break;
		case 'b':
			resp = readline("key: ");
			delete(&tree, resp);
			break;
		case 'c':
			resp = readline("key: ");
			present = lookup(&tree, resp, (void**)&value);
			if (present)
				printf("Value: %d\n", value);
			else 
				printf("Not present\n");
			break;
		default:
			exit(EXIT_SUCCESS);
		}
	}
	exit(EXIT_FAILURE);
#else
	for (i = 0; i < 100; i++) {
		pthread_create(&thread, NULL, run_thread, (void*)1000);
	}
	pthread_exit((void*)0);
#endif
}
