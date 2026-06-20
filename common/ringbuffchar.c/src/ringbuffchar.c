/*
 * ringbuffchar.c
 *
 *  Created on: May 22, 2023
 *      Author: mannk
 */


#include "ringbuffchar.h"

#define RINGBUFF_OVERWRITE

ringnode* node_new(char* val) {
	ringnode* self;
	if(!(self = malloc(sizeof(ringnode)))){
		//printf("%p",self);
		return NULL;
	}
	self->next = NULL;
	self->str = val;

	return self;
}

/*
 * use for free node after use ( use after detach it from list)
 *
 */
void freenode(ringnode* node, ringbuff* ref){
	if(node){
		if(ref->freefunc) ref->freefunc(node->str);
		free(node);
	}
}

/*
 * - This will use as much RAM as it need for store nodes but
 * not much than it's cap.
 * - freefunc: is NULL if use node_new("string");
 * - freefunc: is free() or include free() if val use malloc/calloc to allocate memory.
 */
ringbuff* ringbuff_init(uint32_t cap, void (*freefunc)(void *val)) {
	ringbuff* self;
	if((self = malloc(sizeof(ringbuff))) == NULL) {
		return NULL;
	}
	self->isBusy = TRUE;
	self->capacity = cap;
	self->len = 0;
	self->head = NULL;
	self->tail = NULL;
	self->isBusy = FALSE;
	self->freefunc = freefunc;
	return self;
}

rb_err_t ringbuff_destroy(ringbuff* self){
	if(self->isBusy == TRUE)
		return RB_BUSY;

	self->isBusy = TRUE;
	uint32_t currentLen = self->len;
	ringnode* this = self->head;
	ringnode* next;
	while(currentLen--){
		next = this->next;
		if(self->freefunc) self->freefunc(this->str);
		free(this);
		this=next;
	}
	free(self);
	return RB_SUCCESS;
}

rb_err_t ringbuff_reset(ringbuff* self){
	if(self->isBusy == TRUE)
		return RB_BUSY;

	self->isBusy = TRUE;
	uint32_t currentLen = self->len;
	ringnode* this = self->head;
	ringnode* next;
	while(currentLen--){
		next = this->next;
		if(self->freefunc) self->freefunc(this->str);
		free(this);
		this=next;
	}
	self->len = 0;
	self->head = self->tail = NULL;
	return RB_SUCCESS;
}

uint8_t ringbuff_full(ringbuff* self){
	if(self->len == self->capacity){
		return TRUE;
	}
	return FALSE;
}

uint8_t ringbuff_empty(ringbuff* self){
	if(self->len == 0){
		return TRUE;
	}
	return FALSE;
}


rb_err_t ringbuff_put(ringbuff* self, ringnode* node) {
	rb_err_t state;
	if(self->isBusy == TRUE)
		return RB_BUSY;
	if(!node)
		return ERROR;
	state = RB_SUCCESS;

	if(ringbuff_empty(self)){
		self->len++;
		self->head = self->tail = node;
	}
	else if(!ringbuff_full(self)){
		self->tail->next = node;
		self->tail = node;
		self->len++;
	}
	else {	// ring buffer is full
#ifdef RINGBUFF_OVERWRITE
		state = RB_OVERWRITE;
		self->tail->next = node;
		self->tail = node;
		ringnode* newhead = self->head->next;
		// free head node
		ringnode* oldhead= self->head;
		if(self->freefunc) self->freefunc(oldhead->str);
		free(oldhead);
		self->head = newhead;

#else
		/*
		 * block until another one get, so ring buffer have space.
		 */
		while(ringbuff_full(self))
			;
		++(self->len);
		self->tail->next = node;
		self->tail = node;
#endif
	}

	self->isBusy = FALSE;
	return state;
}

/*
 * - return data from head node and free head node data[fix_size]
 * - auto free node after get it.
 */
rb_err_t ringbuff_get(ringbuff* self, char* str, uint32_t n){
	if(self->isBusy == TRUE){
		str = NULL;
		return RB_BUSY;
	}
	if(ringbuff_empty(self)){
		str = NULL;
		return RB_EMPTY;
	}
	ringnode* oldhead = self->head;
	ringnode* newhead = self->head->next;

	str_cpy_n(str, oldhead->str, n);
	self->head = newhead;
	self->len--;
	free(oldhead->str);
	free(oldhead);

	self->isBusy = FALSE;
	return RB_SUCCESS;
}

findResult ringbuff_find(ringbuff* self, char* str){
	findResult result;
	result.first_found_index = 0;
	result.occurentces = 0;
	if(self->isBusy == TRUE){
		result.errcode = RB_BUSY;
		return result;
	}
	if(ringbuff_empty(self)){
		result.errcode = RB_EMPTY;
		return result;
	}
	uint8_t founded = 0;
	ringnode* node = self->head;

	for (uint32_t i = 0; i< self->len; i++){
		if((str_compare_n(str, node->str, str_len(str))) == 0) {
			if(!founded){
				result.first_found_index = i;
				founded = 1;
			}
			result.occurentces++;
		}
		node=node->next;
	}
	result.errcode = RB_SUCCESS;
	self->isBusy = FALSE;
	return result;
}
