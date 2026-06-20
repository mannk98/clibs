/*
 * ringbuffchar.h
 *
 *  Created on: May 22, 2023
 *      Author: mannk
 */

#ifndef SRC_RINGBUFFCHAR_H_
#define SRC_RINGBUFFCHAR_H_

#include <stdlib.h>	// for malloc, free, ...
#include <stdint.h> // for uint8_t, ...
#include "str_utils.h"

//#define NULL ((void*)0)
#define TRUE 0x01
#define FALSE 0x00

typedef enum {
	RB_SUCCESS = 0, RB_OVERWRITE = -1, RB_EMPTY = -2, ERROR = -3, RB_BUSY = -4, RB_NOTFOUND = -5
} rb_err_t;

typedef struct ringBuffNode {
	char* str;
	struct ringBuffNode* next;
} ringnode;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	uint8_t isBusy;
	ringnode* head;
	ringnode* tail;
	void (*freefunc)(void *val); // use for free (void* val) in node, depend on type data u use for for node;
} ringbuff;

typedef struct resultOfFindFunc{
	uint32_t occurentces;
	uint32_t first_found_index; // start search from head ( index start from 0)
	rb_err_t errcode;
} findResult;

ringnode* node_new(char* val);
ringbuff* ringbuff_init(uint32_t size, void (*freefunc)(void *val));
rb_err_t ringbuff_destroy(ringbuff* self);
rb_err_t ringbuff_free(ringbuff* self);
rb_err_t ringbuff_put(ringbuff* self, ringnode* node);
rb_err_t ringbuff_get(ringbuff* self, char* str, uint32_t bitread);
findResult ringbuff_find(ringbuff* self, char* str);

uint8_t ringbuff_full(ringbuff* self);
uint8_t ringbuff_empty(ringbuff* self);


#endif /* SRC_RINGBUFFCHAR_H_ */
