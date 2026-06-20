/*
 * main.c
 *
 *  Created on: May 17, 2023
 *      Author: mannk
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
//#include <stdlib.h>

#include "src/dlinklist.h"


int comparestring(char* a, char* b) {
	return strcmp(a, b);
}

void freeProxy(void *val) {
	free(val);
}

void test_node_new(){
	node* testnode;
	char* test = "I can go on for hours";
	testnode = node_new(test);
	assert(testnode->val == test);
	assert(comparestring(test, testnode->val) == 0);
	printf("function %s pass, val is %s\n", __FUNCTION__, (char*)testnode->val);
}

void test_list_tpush() {
	// Setup
	linklist* list = list_init(200, NULL);
	node* a = node_new("a");
	node* b = node_new("b");
	node* c = node_new("c");
	//printf("%p\n",a->val);

	// a b c
	list_tput(list, a);
	list_tput(list, b);
	list_tput(list, c);

	// Assertions
	assert(a == list->head);
	assert(c == list->tail);
	assert(3 == list->len);
	assert(b == a->next);
	assert(NULL == a->prev);
	assert(c == b->next);
	assert(a == b->prev);
	assert(NULL == c->next);
	assert(b == c->prev);
	//printf("%p",list->tail->val);

	assert(list_destroy(list) == RB_SUCCESS);
	printf("function %s pass\n", __FUNCTION__);
}

void test_list_tpush_with_mallocval() {
	// Setup
	linklist* list = list_init(200, free);
	char* vala = malloc(sizeof(char)*10);
	char* valb = malloc(sizeof(char)*10);
	char* valc = malloc(sizeof(char)*10);

	strcpy(vala, "ab");
	strcpy(valb, "bc");
	strcpy(valc, "cd");

	node* a = node_new(vala);
	node* b = node_new(valb);
	node* c = node_new(valc);
	// a b c
	list_tput(list, a);
	list_tput(list, b);
	list_tput(list, c);

	//printf("%s",list->tail->val);
	// Assertions
	assert(a == list->head);
	assert(c == list->tail);
	assert(3 == list->len);
	assert(b == a->next);
	assert(NULL == a->prev);
	assert(c == b->next);
	assert(a == b->prev);
	assert(NULL == c->next);
	assert(b == c->prev);
	//printf("%p",list->tail->val);

	assert(list_destroy(list) == RB_SUCCESS);
	printf("function %s pass\n", __FUNCTION__);
}


void test_list_hpush() {
	// Setup
	linklist* list = list_init(200, NULL);
	node* a = node_new("a");
	node* b = node_new("b");
	node* c = node_new("c");

	// a b c
	list_hput(list, a);
	list_hput(list, b);
	list_hput(list, c);

	// Assertions
	assert(c == list->head);
	assert(a == list->tail);
	assert(3 == list->len);
	assert(b == c->next);
	assert(NULL == c->prev);
	assert(a == b->next);
	assert(c == b->prev);
	assert(NULL == a->next);
	assert(b == a->prev);

	assert(list_destroy(list) == RB_SUCCESS);
	printf("function %s pass\n", __FUNCTION__);
}

void test_destroy(){
	linklist* list = list_init(200, NULL);
	assert(list_destroy(list) == RB_SUCCESS);

	//list->len = 100;
	//printf("%d\n", list->len);

	list = list_init(200, NULL);
	node* a = node_new("a");
	node* b = node_new("b");
	node* c = node_new("c");
	list_hput(list, a);
	list_hput(list, b);
	list_hput(list, c);
	printf("function %s: list element before destroy:\n", __FUNCTION__);
	printf("%p\n",list->capacity);
	printf("%p\n",list->head->val);
	printf("%s\n",list->head->val);
	assert(list_destroy(list) == RB_SUCCESS);
	printf("function %s: list element after destroy:\n", __FUNCTION__);
	printf("%p\n",list->capacity);
	printf("%p\n",list->head->val);
	printf("%s\n",list->head->val);
	//assert(strcmp((char*)list->head->val, "c")); return segmentation fault
	printf("function %s pass\n", __FUNCTION__);
}

void test_destroy_with_mallocval(){
	linklist* list = list_init(200, free);
	assert(list_destroy(list) == RB_SUCCESS);

	list_init(200, free);
	char* vala = malloc(sizeof(char)*10);
	char* valb = malloc(sizeof(char)*10);
	char* valc = malloc(sizeof(char)*10);
	//printf("%s\n",a->val);
	node* a = node_new(vala);
	node* b = node_new(valb);
	node* c = node_new(valc);
	// a b c
	list_tput(list, a);
	list_tput(list, b);
	list_tput(list, c);

	assert(list_destroy(list) == RB_SUCCESS);
	//assert(strcmp((char*)list->head->val, "c")); return segmentation fault
	printf("function %s pass\n", __FUNCTION__);
}

void test_reset(){
	linklist* list = list_init(200, NULL);

	node* a = node_new("a");
	node* b = node_new("b");
	node* c = node_new("c");
	node* d = node_new("d");
	node* e = node_new("e");

	list_hput(list, a);
	list_hput(list, b);
	list_hput(list, c);
	list_hput(list, d);
	list_hput(list, e);

	assert(list->len == 5);

	assert(list_reset(list) == RB_SUCCESS);
	assert(list->len == 0);
	assert(listEmpty(list) == TRUE);
	assert(list_destroy(list) == RB_SUCCESS);
	//assert(strcmp((char*)list->head->val, "c")); return segmentation fault
	printf("function %s pass\n", __FUNCTION__);
}

void test_pop(){
	linklist* list = list_init(200, NULL);

	node* a = node_new("a");
	node* b = node_new("b");
	node* c = node_new("c");
	node* d = node_new("d");
	node* e = node_new("e");

	list_hput(list, a);
	list_hput(list, b);
	list_hput(list, c);
	list_hput(list, d);
	list_hput(list, e);

	list_hpop(list);
	list_hpop(list);
	assert(list->len == 3);
	assert(list->head == c);
	list_tpop(list);
	list_tpop(list);
	assert(list->len ==1);
	assert(list->head == c);
	assert(list->tail == c);
	assert(list->tail->next == NULL);
	assert(list->tail->prev == NULL);
	assert(list->head->next == NULL);
	assert(list->head->next == NULL);
	assert(list_destroy(list) == RB_SUCCESS);
	//assert(strcmp((char*)list->head->val, "c")); return segmentation fault
	printf("function %s pass\n", __FUNCTION__);
}

void test_find(){
	linklist* list = list_init(200, NULL);
	nodeinfo nodeinfo;

	node* a = node_new("ab");
	node* b = node_new("b");
	node* c = node_new("c");
	node* d = node_new("d");
	node* e = node_new("e");
	node* f = node_new("ab");

	list_hput(list, a);
	list_hput(list, b);
	list_hput(list, c);
	list_hput(list, d);
	list_hput(list, e);
	list_hput(list, f);

	nodeinfo = find(list, "ab", comparestring);
	assert(nodeinfo.occurences == 2);
	assert (nodeinfo.lastlocation == a);

	list_tpop(list);
	nodeinfo = find(list, "ab", comparestring);
	assert(nodeinfo.occurences == 1);
	assert (nodeinfo.lastlocation == f);
	assert(list_destroy(list) == RB_SUCCESS);
	//assert(strcmp((char*)list->head->val, "c")); return segmentation fault
	printf("function %s pass\n", __FUNCTION__);
}

int main(){
	test_node_new();
	test_list_tpush();
	test_list_tpush_with_mallocval();
	test_list_hpush();
	test_destroy();
	test_destroy_with_mallocval();
	test_reset();
	test_pop();
	test_find();
	return 0;
}

