#ifndef CLIBS_PQUEUE_H
#define CLIBS_PQUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* pqueue — array-backed binary min-heap (priority queue).
 *
 * Keyed by int32_t (lower key = higher priority) with an opaque void*
 * payload. No comparator function pointer, no vtable: ordering is fixed
 * to ascending int32_t key. Caller supplies the backing storage; the
 * library never allocates. Every method is NULL-self-safe.
 */

/* A single heap entry. Lower key = higher priority. `value` is opaque
 * payload owned by the caller (may be NULL). */
typedef struct {
    int32_t key;
    void   *value;
} pqueue_node;

/* The priority queue object. Fields are private — use the methods. */
typedef struct {
    pqueue_node *nodes; /* caller storage: cap entries */
    size_t       cap;   /* capacity in entries */
    size_t       len;   /* current entry count */
} pqueue;

/* Initialise `self` to an empty heap backed by `storage` (cap entries).
 * No-op if `self` is NULL. */
void pqueue_init(pqueue *self, pqueue_node *storage, size_t cap);

/* Insert (key, value), sifting up to restore the heap order.
 * Returns false if `self` is NULL or the heap is full. */
bool pqueue_push(pqueue *self, int32_t key, void *value);

/* Remove the minimum-key entry. Writes its key to *key_out and value to
 * *val_out when those pointers are non-NULL (either may be NULL).
 * Returns false (outs untouched) if `self` is NULL or the heap is empty. */
bool pqueue_pop_min(pqueue *self, int32_t *key_out, void **val_out);

/* Read the minimum-key entry without removing it. Writes key/value to the
 * non-NULL out pointers. Returns false (outs untouched) if `self` is NULL
 * or the heap is empty. */
bool pqueue_peek_min(const pqueue *self, int32_t *key_out, void **val_out);

/* Number of entries currently stored. Returns 0 if `self` is NULL. */
size_t pqueue_len(const pqueue *self);

/* True if the heap holds no entries (also true if `self` is NULL). */
bool pqueue_is_empty(const pqueue *self);

#endif /* CLIBS_PQUEUE_H */
