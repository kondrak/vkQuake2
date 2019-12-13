#include "g_local.h"
#include "z_list.h"
#include <stdlib.h>

typedef struct listNode_s
{
	void *data;
	struct listNode_s *next;
	struct listNode_s *prev;
} listNode_t;

void initializeList(list_t *list)
{
	// initialize the list to be empty
	memset(list, 0, sizeof(list_t));
	list->head = NULL;
	list->tail = NULL;
	list->numNodes = 0;
	list->curNode = 0;
	list->currentNode = NULL;
}

void emptyList(list_t *list)
{
	while (list->head != NULL)
	{
		// get the node
		listNode_t *node = list->head;
		list->head = node->next;

		// free it up
		Z_FREE(node);
	}
	initializeList(list);
}

int listLength(list_t const *list)
{
	return list->numNodes;
}

void addHead(list_t *list, void *data)
{
	// add a node to the head of the list
	listNode_t *node = (listNode_t*)Z_MALLOC(sizeof(listNode_t));

	// fill it and put it in the front of the list
	node->data = data;
	node->next = list->head;
	node->prev = NULL;
	list->head = node;
	list->numNodes++;

	if (list->numNodes == 1)
		list->tail = list->head;

	// reset the currentNode
	list->currentNode = list->head;
	list->curNode = 0;
}

void addTail(list_t *list, void *data)
{
	// add a node to the head of the list
	listNode_t *node = (listNode_t*)Z_MALLOC(sizeof(listNode_t));

	// fill it and put it in the front of the list
	node->data = data;
	node->next = NULL;
	node->prev = list->tail;
	if (list->tail != NULL)
		list->tail->next = node;
	list->tail = node;
	list->numNodes++;

	if (list->numNodes == 1)
		list->head = list->tail;

	// reset the currentNode
	list->currentNode = list->head;
	list->curNode = 0;
}

void *getItem(list_t *list, int num)
{
	listNode_t *node = NULL;
	int indexNum = 0;
	int diffH = 0;
	int diffT = 0;
	int diffC = 0;
	
	// safety check
	if (num >= list->numNodes || num < 0)
		return NULL;

	// which is closer, head, tail or currentNode?
	diffH = num;
	diffT = list->numNodes - num - 1;
	diffC = abs(list->curNode - num);

	if (diffH < diffT &&
		diffH < diffC)
	{
		node = list->head;
		indexNum = 0;
	}
	else if (diffT < diffC)
	{
		node = list->tail;
		indexNum = list->numNodes - 1;
	}
	else 
	{
		node = list->currentNode;
		indexNum = list->curNode;
	}

	// go thru the list searching
	while (indexNum != num)
	{
		if (indexNum < num)
		{
			indexNum++;
			node = node->next;
		}
		else if (indexNum > num)
		{
			indexNum--;
			node = node->prev;
		}
	}

	// update the current node
	list->currentNode = node;
	list->curNode = indexNum;

	// return the current node
	return list->currentNode->data;
}

void removeItem(list_t *list, int num)
{
	// find the item
	listNode_t *node = NULL;
	void *item = getItem(list, num);

	if (item == NULL)
		return;

	// ok, currentNode points to the node
	// remove it
	node = list->currentNode;

	// previous?
	if (node->prev == NULL) // front of the list
		list->head = node->next;
	else
		node->prev->next = node->next;
	
	if (node->next == NULL) // tail of the list
		list->tail = node->prev;
	else
		node->next->prev = node->prev;
	
	// delete the node
	Z_FREE(node);
	list->numNodes--;

	// reset the current node
	list->currentNode = list->head;
	list->curNode = 0;
}

void copyList(list_t *src, list_t *dest)
{
	// get and put the data
	int i = 0;

	// empty the destination
	emptyList(dest);
	for (i = 0; i < src->numNodes; i++)
	{
		void *item = getItem(src, i);
		addTail(dest, item);
	}
}

void stealList(list_t *src, list_t *dest)
{
	// copy over the pointer info
	dest->head = src->head;
	dest->tail = src->tail;
	dest->numNodes = src->numNodes;
	dest->curNode = src->curNode;
	dest->currentNode = src->currentNode;

	// init src
	initializeList(src);
}

int findIndex(list_t *list, void *data)
{
	listNode_t *node = list->head;
	int i = 0;
	while (node != NULL)
	{
		if (node->data == data)
			return i;

		i++;
		node = node->next;
	}

	// failure
	return -1;
}