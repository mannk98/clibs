/*
 * ringbuff.h — malloc-free byte FIFO over caller-provided storage.
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli.
 */
#ifndef CLIBS_RINGBUFF_H
#define CLIBS_RINGBUFF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;

typedef struct {
    uint8_t *buf;       /* caller storage, >= cap bytes */
    uint16_t cap;       /* capacity in bytes */
    uint16_t head;      /* index of oldest byte */
    uint16_t count;     /* bytes stored */
    bool     overwrite; /* when full: evict oldest (true) vs reject (false) */
} ringbuf;

void     ringbuf_init(ringbuf *self, uint8_t *storage, uint16_t cap, bool overwrite);
rb_err   ringbuf_put(ringbuf *self, uint8_t byte);
rb_err   ringbuf_get(ringbuf *self, uint8_t *out);    /* out may be NULL to drop */
rb_err   ringbuf_peek(const ringbuf *self, uint8_t *out);
rb_err   ringbuf_put_string(ringbuf *self, const char *s); /* '\0'-terminated; RB_FULL if it filled mid-string (bytes already inserted are kept) */
uint16_t ringbuf_get_bytes(ringbuf *self, uint8_t *out, uint16_t max); /* #copied */
uint16_t ringbuf_count(const ringbuf *self);
/* A NULL/invalid buffer reports as BOTH empty and full (fail-safe for readers and writers). */
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full(const ringbuf *self);
void     ringbuf_clear(ringbuf *self);

#endif /* CLIBS_RINGBUFF_H */
