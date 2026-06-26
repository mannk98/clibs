/*
 * ringbuff.c — malloc-free byte FIFO over caller-provided storage.
 */
#include "ringbuff.h"

void ringbuf_init(ringbuf *self, uint8_t *storage, uint16_t cap, bool overwrite) {
    if (self == NULL) return;
    if (storage == NULL || cap == 0) {     /* invalid config: safe empty state */
        self->buf = NULL; self->cap = 0; self->head = 0; self->count = 0;
        self->overwrite = false;
        return;
    }
    self->buf = storage;
    self->cap = cap;
    self->head = 0;
    self->count = 0;
    self->overwrite = overwrite;
}

rb_err ringbuf_put(ringbuf *self, uint8_t byte) {
    if (self == NULL || self->buf == NULL || self->cap == 0) return RB_INVALID;
    rb_err rc = RB_OK;
    if (self->count == self->cap) {
        if (!self->overwrite) return RB_FULL;
        self->head = (uint16_t)((self->head + 1) % self->cap); /* evict oldest */
        self->count--;
        rc = RB_OVERWRITE;
    }
    uint16_t tail = (uint16_t)(((uint32_t)self->head + self->count) % self->cap);
    self->buf[tail] = byte;
    self->count++;
    return rc;
}

rb_err ringbuf_get(ringbuf *self, uint8_t *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL) *out = self->buf[self->head];
    self->head = (uint16_t)((self->head + 1) % self->cap);
    self->count--;
    return RB_OK;
}

rb_err ringbuf_peek(const ringbuf *self, uint8_t *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL) *out = self->buf[self->head];
    return RB_OK;
}

rb_err ringbuf_put_string(ringbuf *self, const char *s) {
    if (self == NULL || self->buf == NULL || s == NULL) return RB_INVALID;
    rb_err rc = RB_OK;
    for (; *s != '\0'; s++) {
        rb_err r = ringbuf_put(self, (uint8_t)*s);
        if (r == RB_FULL) return RB_FULL;
        if (r == RB_OVERWRITE) rc = RB_OVERWRITE;
    }
    return rc;
}

uint16_t ringbuf_get_bytes(ringbuf *self, uint8_t *out, uint16_t max) {
    if (self == NULL || self->buf == NULL || out == NULL) return 0;
    uint16_t n = 0;
    while (n < max && self->count > 0) {
        out[n] = self->buf[self->head];
        self->head = (uint16_t)((self->head + 1) % self->cap);
        self->count--;
        n++;
    }
    return n;
}

uint16_t ringbuf_count(const ringbuf *self)    { return self ? self->count : 0; }
bool     ringbuf_is_empty(const ringbuf *self) { return self ? self->count == 0 : true; }
bool     ringbuf_is_full(const ringbuf *self)  { return self ? self->count == self->cap : true; }

void ringbuf_clear(ringbuf *self) {
    if (self == NULL) return;
    self->head = 0;
    self->count = 0;
}
