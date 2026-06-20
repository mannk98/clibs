#ifndef CLIBS_DLL_H
#define CLIBS_DLL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Doubly linked list of opaque void* values over a caller-provided node pool.
 * No malloc: capacity == pool size; an internal free-list recycles nodes.
 * The list never copies/frees the stored payload — the caller owns it.
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli. */

typedef enum { DLL_OK, DLL_FULL, DLL_EMPTY, DLL_NOT_FOUND, DLL_INVALID } dll_err;

typedef struct dll_node {
    void *value;
    struct dll_node *prev;
    struct dll_node *next;
} dll_node;

typedef struct {
    dll_node *pool;   /* caller-provided array of cap nodes */
    dll_node *head;
    dll_node *tail;
    dll_node *free;   /* free-list head */
    uint16_t  count;
    uint16_t  cap;
} dll_list;

void     dll_init(dll_list *self, dll_node *pool, uint16_t cap);
dll_err  dll_push_head(dll_list *self, void *value);
dll_err  dll_push_tail(dll_list *self, void *value);
dll_err  dll_pop_head(dll_list *self, void **out);  /* out may be NULL */
dll_err  dll_pop_tail(dll_list *self, void **out);  /* out may be NULL */

/* match(stored_value, key) returns 0 when equal (strcmp-style).
 * On hit: returns DLL_OK and (if found != NULL) *found = node.
 * On miss: returns DLL_NOT_FOUND and *found = NULL. */
dll_err  dll_find(const dll_list *self, const void *key,
                  int (*match)(const void *a, const void *b), dll_node **found);

uint16_t dll_count(const dll_list *self);
bool     dll_is_empty(const dll_list *self);
bool     dll_is_full(const dll_list *self);
void     dll_clear(dll_list *self);

#endif /* CLIBS_DLL_H */
