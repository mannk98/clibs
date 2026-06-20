/*
 * lister.c
 *
 *  Created on: May 17, 2023
 *      Author: mannk
 */

#include "dlinklist.h"

node* node_new(void* val) {
	node* self;
	if((self = malloc(sizeof(node))) == NULL){
		return NULL;
	}
	self->next = NULL;
	self->prev = NULL;
	self->val = val;

	return self;
}

/*
 * This will use as much RAM as it need for store nodes but
 * not much than it's cap.
 */
linklist* list_init(uint32_t cap, void (*freefunc)(void *val)) {
	linklist* self;
	if((self = malloc(sizeof(linklist))) == NULL) {
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

llErrTypes list_destroy(linklist *self) {
	if (self->isBusy == TRUE)
		return RB_BUSY;
	self->isBusy = TRUE;
	uint32_t currentLen = self->len;
	node* this = self->head;
	node* next;
	while(currentLen--){
		next = this->next;
		if(self->freefunc) self->freefunc(this->val);
		free(this);
		this=next;
	}
	free(self);
	return RB_SUCCESS;
}

llErrTypes list_reset(linklist *self) {
	if (self->isBusy == TRUE)
		return RB_BUSY;
	self->isBusy = TRUE;
	node* this = self->head;
	node* next;
	uint32_t currentLen = self->len;
	while(currentLen--){
		next = this->next;
		if(self->freefunc) self->freefunc(this->val);
		free(this);
		this=next;
	}
	self->tail = self->head = NULL;
	self->len = 0;

	self->isBusy = FALSE;
	return RB_SUCCESS;
}

llErrTypes list_hput(linklist *self, node* node) {
	if (self->isBusy == TRUE)
		return RB_BUSY;
	if(!node)
		return ERROR;
	if(self->len == self->capacity){
		return RB_FULL;
	}
	self->isBusy = TRUE;
	if(self->len){
		node->next = self->head;
		node->prev= NULL;
		self->head->prev=node;
		self->head = node;
	}
	else {
		self->tail = self->head = node;
		node->next = node->prev = NULL;
	}
	++(self->len);

	self->isBusy = FALSE;
	return RB_SUCCESS;
}

llErrTypes list_tput(linklist* self, node* node) {
	if (self->isBusy == TRUE)
		if (self->isBusy == TRUE)
			return RB_BUSY;
	if(!node)
		return ERROR;
	if(self->len == self->capacity){
		return RB_FULL;
	}
	if(self->len){
		node->next = NULL;
		node->prev= self->tail;
		self->tail->next = node;
		self->tail = node;
	}
	else {
		self->tail = self->head = node;
		node->next = node->prev = NULL;
	}
	++(self->len);

	self->isBusy = FALSE;
	return RB_SUCCESS;
}

llErrTypes list_hpop(linklist* self){
	if (self->isBusy == TRUE)
		return RB_BUSY;
	if(self->len == 0)
		return RB_EMPTY;

	self->isBusy = TRUE;
	if(--self->len){
		self->head = self->head->next;
		free(self->head->prev);
		self->head->prev = NULL;
	} else {
		free(self->head);
		self->head = self->tail = NULL;
	}
	self->isBusy = FALSE;
	return RB_SUCCESS;
}

llErrTypes list_tpop(linklist* self){
	if (self->isBusy == TRUE)
		return RB_BUSY;
	if(self->len == 0)
		return RB_EMPTY;

	self->isBusy = TRUE;
	if(--self->len){
		self->tail = self->tail->prev;
		free(self->tail->next);
		self->tail->next = NULL;
	} else {
		free(self->head);
		self->head = self->tail = NULL;
	}
	self->isBusy = FALSE;
	return RB_SUCCESS;
}

nodeinfo find(linklist* self, void* val, int (*math)(void*, void*)){
	nodeinfo info;
	info.lastlocation = NULL;
	info.occurences=0;
	if (self->isBusy == TRUE){
		info.error = RB_BUSY;
		return info;
	}
	if(self->len == 0) {
		info.error = RB_EMPTY;
		return info;
	}

	self->isBusy = TRUE;
	uint32_t location = 0;
	node* temp;
	temp=self->head;

	while(temp!=NULL){
		if(math(val, temp->val) == 0){
			info.occurences++;
			info.lastlocation = temp;
			temp = temp->next;
		} else {
			temp = temp->next;
		}
		location++;
	}

	self->isBusy = FALSE;
	return info;
}


uint8_t listEmpty(linklist *self) {
	if (self->len == 0 && self->tail == self->head) {
		return TRUE;
	}
	return FALSE;
}

uint8_t listFull(linklist *self) {
	if (self->len == self->capacity) {
		return TRUE;
	}
	return FALSE;
}

