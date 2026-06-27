/* pqueue — array-backed binary min-heap. See pqueue.h for the contract. */
#include "pqueue.h"

/* Swap two heap entries by index. Indices are assumed in bounds. */
static void pq_swap(pqueue *self, size_t i, size_t j)
{
    pqueue_node tmp = self->nodes[i];
    self->nodes[i] = self->nodes[j];
    self->nodes[j] = tmp;
}

/* Restore heap order upward from index `i` (after an insert at the tail). */
static void pq_sift_up(pqueue *self, size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (self->nodes[i].key >= self->nodes[parent].key) {
            break;
        }
        pq_swap(self, i, parent);
        i = parent;
    }
}

/* Restore heap order downward from index `i` (after moving tail to root). */
static void pq_sift_down(pqueue *self, size_t i)
{
    for (;;) {
        size_t left  = 2 * i + 1;
        size_t right = 2 * i + 2;
        size_t smallest = i;

        if (left < self->len &&
            self->nodes[left].key < self->nodes[smallest].key) {
            smallest = left;
        }
        if (right < self->len &&
            self->nodes[right].key < self->nodes[smallest].key) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }
        pq_swap(self, i, smallest);
        i = smallest;
    }
}

void pqueue_init(pqueue *self, pqueue_node *storage, size_t cap)
{
    if (self == NULL) {
        return;
    }
    self->nodes = storage;
    self->cap = cap;
    self->len = 0;
}

bool pqueue_push(pqueue *self, int32_t key, void *value)
{
    if (self == NULL || self->len >= self->cap) {
        return false;
    }
    size_t i = self->len;
    self->nodes[i].key = key;
    self->nodes[i].value = value;
    self->len++;
    pq_sift_up(self, i);
    return true;
}

bool pqueue_pop_min(pqueue *self, int32_t *key_out, void **val_out)
{
    if (self == NULL || self->len == 0) {
        return false;
    }
    if (key_out != NULL) {
        *key_out = self->nodes[0].key;
    }
    if (val_out != NULL) {
        *val_out = self->nodes[0].value;
    }
    self->len--;
    if (self->len > 0) {
        self->nodes[0] = self->nodes[self->len];
        pq_sift_down(self, 0);
    }
    return true;
}

bool pqueue_peek_min(const pqueue *self, int32_t *key_out, void **val_out)
{
    if (self == NULL || self->len == 0) {
        return false;
    }
    if (key_out != NULL) {
        *key_out = self->nodes[0].key;
    }
    if (val_out != NULL) {
        *val_out = self->nodes[0].value;
    }
    return true;
}

size_t pqueue_len(const pqueue *self)
{
    return (self == NULL) ? 0 : self->len;
}

bool pqueue_is_empty(const pqueue *self)
{
    return (self == NULL) ? true : (self->len == 0);
}
