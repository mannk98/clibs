#include "ringbuf.h"
#include <string.h>  /* memcpy */

void ringbuf_init(ringbuf *self, void *storage, uint16_t cap,
                  uint16_t elem_size, bool overwrite) {
    if (self == NULL) return;
    if (storage == NULL || elem_size == 0) {   /* invalid config: safe empty state */
        self->buf = NULL;
        self->cap = 0;
        self->elem_size = 0;
        self->head = 0;
        self->count = 0;
        self->overwrite = false;
        return;
    }
    self->buf = (uint8_t *)storage;
    self->cap = cap;
    self->elem_size = elem_size;
    self->head = 0;
    self->count = 0;
    self->overwrite = overwrite;
}

rb_err ringbuf_put(ringbuf *self, const void *elem) {
    if (self == NULL || elem == NULL || self->buf == NULL || self->cap == 0)
        return RB_INVALID;

    rb_err rc = RB_OK;
    if (self->count == self->cap) {
        if (!self->overwrite) return RB_FULL;
        self->head = (uint16_t)((self->head + 1) % self->cap); /* evict oldest */
        self->count--;
        rc = RB_OVERWRITE;
    }
    uint16_t tail = (uint16_t)(((uint32_t)self->head + self->count) % self->cap);
    memcpy(self->buf + (uint32_t)tail * self->elem_size, elem, self->elem_size);
    self->count++;
    return rc;
}

rb_err ringbuf_get(ringbuf *self, void *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL)
        memcpy(out, self->buf + (uint32_t)self->head * self->elem_size,
               self->elem_size);
    self->head = (uint16_t)((self->head + 1) % self->cap);
    self->count--;
    return RB_OK;
}

rb_err ringbuf_peek(const ringbuf *self, void *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL)
        memcpy(out, self->buf + (uint32_t)self->head * self->elem_size,
               self->elem_size);
    return RB_OK;
}

uint16_t ringbuf_count(const ringbuf *self)    { return self ? self->count : 0; }
bool     ringbuf_is_empty(const ringbuf *self) { return self ? self->count == 0 : true; }
bool     ringbuf_is_full(const ringbuf *self)  { return self ? self->count == self->cap : true; }

void ringbuf_clear(ringbuf *self) {
    if (self == NULL) return;
    self->head = 0;
    self->count = 0;
}
