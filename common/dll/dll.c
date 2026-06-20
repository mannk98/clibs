#include "dll.h"

void dll_init(dll_list *self, dll_node *pool, uint16_t cap) {
    if (self == NULL) return;
    if (pool == NULL) {                 /* invalid config: safe empty state */
        self->pool = NULL;
        self->head = NULL;
        self->tail = NULL;
        self->free = NULL;
        self->count = 0;
        self->cap = 0;
        return;
    }
    self->pool  = pool;
    self->head  = NULL;
    self->tail  = NULL;
    self->free  = NULL;
    self->count = 0;
    self->cap   = cap;
    for (uint16_t i = 0; i < cap; i++) {   /* push every node onto the free-list */
        pool[i].value = NULL;
        pool[i].prev  = NULL;
        pool[i].next  = self->free;
        self->free    = &pool[i];
    }
}

static dll_node *node_alloc(dll_list *self) {
    dll_node *n = self->free;
    if (n == NULL) return NULL;
    self->free = n->next;
    n->prev = NULL;
    n->next = NULL;
    n->value = NULL;
    return n;
}

static void node_release(dll_list *self, dll_node *n) {
    n->prev  = NULL;
    n->value = NULL;
    n->next  = self->free;
    self->free = n;
}

dll_err dll_push_head(dll_list *self, void *value) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == self->cap) return DLL_FULL;
    dll_node *n = node_alloc(self);
    if (n == NULL) return DLL_FULL;
    n->value = value;
    n->prev  = NULL;
    n->next  = self->head;
    if (self->head != NULL) self->head->prev = n;
    self->head = n;
    if (self->tail == NULL) self->tail = n;
    self->count++;
    return DLL_OK;
}

dll_err dll_push_tail(dll_list *self, void *value) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == self->cap) return DLL_FULL;
    dll_node *n = node_alloc(self);
    if (n == NULL) return DLL_FULL;
    n->value = value;
    n->next  = NULL;
    n->prev  = self->tail;
    if (self->tail != NULL) self->tail->next = n;
    self->tail = n;
    if (self->head == NULL) self->head = n;
    self->count++;
    return DLL_OK;
}

dll_err dll_pop_head(dll_list *self, void **out) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == 0) return DLL_EMPTY;
    dll_node *n = self->head;
    if (out != NULL) *out = n->value;
    self->head = n->next;
    if (self->head != NULL) self->head->prev = NULL;
    else self->tail = NULL;
    node_release(self, n);
    self->count--;
    return DLL_OK;
}

dll_err dll_pop_tail(dll_list *self, void **out) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == 0) return DLL_EMPTY;
    dll_node *n = self->tail;
    if (out != NULL) *out = n->value;
    self->tail = n->prev;
    if (self->tail != NULL) self->tail->next = NULL;
    else self->head = NULL;
    node_release(self, n);
    self->count--;
    return DLL_OK;
}

dll_err dll_find(const dll_list *self, const void *key,
                 int (*match)(const void *, const void *), dll_node **found) {
    if (self == NULL || match == NULL) return DLL_INVALID;
    for (dll_node *n = self->head; n != NULL; n = n->next) {
        if (match(n->value, key) == 0) {
            if (found != NULL) *found = n;
            return DLL_OK;
        }
    }
    if (found != NULL) *found = NULL;
    return DLL_NOT_FOUND;
}

uint16_t dll_count(const dll_list *self)    { return self ? self->count : 0; }
bool     dll_is_empty(const dll_list *self) { return self ? self->count == 0 : true; }
bool     dll_is_full(const dll_list *self)  { return self ? self->count == self->cap : true; }

void dll_clear(dll_list *self) {
    if (self == NULL) return;
    while (self->head != NULL) {
        dll_node *n = self->head;
        self->head = n->next;
        node_release(self, n);
    }
    self->tail  = NULL;
    self->count = 0;
}
