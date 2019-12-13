#ifndef __Z_LIST_H
#define __Z_LIST_H

/*
	Linked List structure

	All items contained in this list but be pointers.  The list does not claim 
	"ownership" of the data, so it is not the responsibility of the list to 
	free the memory allocated by the data.  It will, however, free the memory taken 
	by the nodes
*/

// dummy for internals
struct listNode_s;

typedef struct list_s
{
	// start of the list
	struct listNode_s *head;
	struct listNode_s *tail;

	// # of nodes in the list
	int numNodes;

	// current node
	int curNode;
	struct listNode_s *currentNode;
} list_t;

// init/cleanup
void initializeList(list_t *list);
void emptyList(list_t *list);

// get information about the list
int listLength(list_t const *list);

// add items to the list
void addHead(list_t *list, void *data);
void addTail(list_t *list, void *data);

// remove items from the list
void removeItem(list_t *list, int num);

// get items from the list (0 based)
void *getItem(list_t *list, int num);
int findIndex(list_t *list, void *data);

// data transfer
void copyList(list_t *src, list_t *dest);
void stealList(list_t *src, list_t *dest);

#endif