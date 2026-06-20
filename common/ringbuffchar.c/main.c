/*
 * main.c
 *
 *  Created on: May 22, 2023
 *      Author: mannk
 */

#include <stdio.h>
#include "src/ringbuffchar.h"
#include <assert.h>
#include <unistd.h>
#include <string.h>

void testput(){
	rb_err_t error_code;
	ringbuff* rb = ringbuff_init(5, NULL);

	ringnode* a = node_new("a");
	ringnode* b = node_new("b");
	ringnode* c = node_new("c");
	ringnode* d = node_new("d");
	ringnode* e = node_new("e");
	ringnode* f = node_new("f");
	ringnode* g = node_new("g");

	error_code = ringbuff_put(rb, a);
	//printf("%s\n", result.ringbuffval);
	error_code = ringbuff_put(rb, b);
	error_code = ringbuff_put(rb, c);
	error_code = ringbuff_put(rb, d);
	error_code = ringbuff_put(rb, e);

	assert(rb->head == a);
	assert(rb->tail == e);
	assert(rb->len == 5);

	error_code = ringbuff_put(rb, f);
	error_code = ringbuff_put(rb, g);
	assert(rb->head == c);
	assert(rb->tail == g);
	assert(rb->len == 5);
	assert(rb->head->next == d);

	error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS );
	printf("function %s is passed.\n", __FUNCTION__);
}

void testputfreeAnddestroy(){
	rb_err_t error_code;
	ringbuff* rb = ringbuff_init(5, free);

	char* vala = malloc(10*sizeof(char));
	char* valb = malloc(10*sizeof(char));
	char* valc = malloc(10*sizeof(char));
	char* vald = malloc(10*sizeof(char));
	char* vale = malloc(10*sizeof(char));
	char* valf = malloc(10*sizeof(char));
	char* valg = malloc(10*sizeof(char));

	ringnode* a = node_new(vala);
	ringnode* b = node_new(valb);
	ringnode* c = node_new(valc);
	ringnode* d = node_new(vald);
	ringnode* e = node_new(vale);
	ringnode* f = node_new(valf);
	ringnode* g = node_new(valg);

	error_code = ringbuff_put(rb, a);
	//printf("%s\n", result.ringbuffval);
	error_code = ringbuff_put(rb, b);
	error_code = ringbuff_put(rb, c);
	error_code = ringbuff_put(rb, d);
	error_code = ringbuff_put(rb, e);

	assert(rb->head == a);
	assert(rb->tail == e);
	assert(rb->len == 5);

	error_code = ringbuff_put(rb, f);
	error_code = ringbuff_put(rb, g);
	assert(rb->head == c);
	assert(rb->tail == g);
	assert(rb->len == 5);
	assert(rb->head->next == d);

	error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS );
	/*	printf("after destroy:\n");
	printf("%p\n",rb->head);
	printf("%d\n", rb->capacity);
	printf("%s\n", rb->tail->val);
	printf("%s\n", rb->head->val);*/
	printf("function %s is passed.\n", __FUNCTION__);
}

void testget(){
	rb_err_t error_code;
	char getresult[30];
	ringbuff* rb = ringbuff_init(5, free);

	char* vala = malloc(10*sizeof(char));
	char* valb = malloc(10*sizeof(char));
	char* valc = malloc(10*sizeof(char));
	char* vald = malloc(10*sizeof(char));
	char* vale = malloc(1*sizeof(int));
	char* valf = malloc(10*sizeof(char));
	char* valg = malloc(10*sizeof(char));

	ringnode* a = node_new(vala);
	ringnode* b = node_new(valb);
	ringnode* c = node_new(valc);
	ringnode* d = node_new(vald);
	ringnode* e = node_new(vale);
	ringnode* f = node_new(valf);
	ringnode* g = node_new(valg);

	strcpy(c->str,"just do it");
	strcpy(d->str,"just do it");
	*((int*)(e->str)) = 10;

	error_code = ringbuff_put(rb, a);
	//printf("%s\n", result.ringbuffval);
	error_code = ringbuff_put(rb, b);
	error_code = ringbuff_put(rb, c);
	error_code = ringbuff_put(rb, d);
	error_code = ringbuff_put(rb, e);
	error_code = ringbuff_put(rb, f);
	error_code = ringbuff_put(rb, g);

	error_code = ringbuff_get(rb, getresult, 30);
	assert(rb->len == 4);
	printf("%s\n", getresult);

	error_code = ringbuff_get(rb, getresult, 30);
	assert(rb->len == 3);
	printf("%s\n", getresult);

	error_code = ringbuff_get(rb, getresult, 30);
	assert(rb->len == 2);
	assert(rb->head == f);
	printf("%d\n", *((int*)(getresult)));


	error_code = ringbuff_get(rb, getresult, 30);
	error_code = ringbuff_get(rb, getresult, 30);
	//error_code = ringbuff_get(rb, getresult, 30);
	assert(rb->len == 0);
	error_code = ringbuff_get(rb, getresult, 30);
	assert(error_code == RB_EMPTY);

	error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS);
	printf("function %s is passed.\n", __FUNCTION__);
}

void testfind(){
	findResult result;
	ringbuff* rb = ringbuff_init(5, NULL);

	ringnode* a = node_new("ab");
	ringnode* b = node_new("bc");
	ringnode* c = node_new("cd");
	ringnode* d = node_new("de");
	ringnode* e = node_new("ef");
	ringnode* f = node_new("fg");
	ringnode* g = node_new("gh");

	rb_err_t error_code = ringbuff_put(rb, a);
	//printf("%s\n", result.ringbuffval);
	error_code = ringbuff_put(rb, b);
	error_code = ringbuff_put(rb, c);
	error_code = ringbuff_put(rb, d);
	error_code = ringbuff_put(rb, e);
	error_code = ringbuff_put(rb, f);
	error_code = ringbuff_put(rb, g);
	//strcpy(rb->head->str,"asdf");

	result = ringbuff_find(rb, "fg");
	assert(result.errcode == RB_SUCCESS);
	assert(result.first_found_index == 3);
	//printf("%d", result.first_found_index100000);
	assert(result.occurentces == 1);

	error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS);
	printf("function %s is passed.\n", __FUNCTION__);
}

void testDestroy1(){
	ringbuff* rb = ringbuff_init(10000000, NULL);
	char str[20]= "just do it for fun!\0";
	for(uint32_t i=0; i< 10000000; i++){
		ringnode* node = node_new(str);
		ringbuff_put(rb, node);
	}
	printf("Follow RAM status...\n");
	sleep(10);
	rb_err_t error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS);
	printf("function %s is passed.\n", __FUNCTION__);
}

void testDestroy2(){
	ringbuff* rb = ringbuff_init(10000000, NULL);
	char str[20]= "just do it for fun!\0";
	for(uint32_t i=0; i< 10000000; i++){
		char* temp = malloc(sizeof(char)*20);
		strcpy(temp,str);
		ringnode* node = node_new(temp);
		ringbuff_put(rb, node);
	}
	printf("Follow RAM status...\n");
	sleep(10);
	rb_err_t error_code = ringbuff_destroy(rb);
	assert(error_code == RB_SUCCESS);
	printf("function %s is passed.\n", __FUNCTION__);
}

int main(){
	testput();
	testputfreeAnddestroy();
	testget();
	testfind();
	testDestroy1();
	testDestroy2();
	return 0;
}

