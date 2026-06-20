/*
 * lister.h
 *
 *  Created on: May 17, 2023
 *      Author: mannk
 */

#ifndef SRC_DLINKLIST_H__
#define SRC_DLINKLIST_H_H_

#include <stdlib.h>	// for malloc, free, ...
#include <stdint.h>

//#define NULL ((void*)0)
#define TRUE 0x01
#define FALSE 0x00

typedef enum {
	RB_SUCCESS = 0, RB_FULL = -1, RB_EMPTY = -2, ERROR = -3, RB_BUSY = -4, RB_NOTFOUND = -5
} llErrTypes;

typedef struct linknode {
	void* val;
	struct linknode* next;
	struct linknode* prev;
} node;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	uint8_t isBusy;
	node* head;
	node* tail;
	int (*match)(void *a, void *b);	// use for compare in find
	void (*freefunc)(void *val); // use for free (void* val) in node, depend on type data u use for for node;
} linklist;

// return value of find function: info of find node function.
typedef struct nodeFindInfo{
	uint32_t occurences;
	node* lastlocation;
	llErrTypes error;		// error code of ringbuff
} nodeinfo;

node* node_new(void* val);
linklist* list_init(uint32_t cap, void (*freefunc)(void *val));
llErrTypes list_destroy(linklist* self);
llErrTypes list_reset(linklist* self);

llErrTypes list_hput(linklist* self, node* node);
llErrTypes list_tput(linklist* self, node* node);
llErrTypes list_hpop(linklist* self);
llErrTypes list_tpop(linklist* self);

nodeinfo find(linklist* self, void* val, int (*math)(void*, void*));
uint8_t listEmpty(linklist *self);
uint8_t listFull(linklist *self);

#endif /SRC_DLINKLIST_H__H_ */
