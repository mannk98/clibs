#ifndef CLIBS_RINGBUF_H
#define CLIBS_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Generic fixed-capacity FIFO over caller-provided storage. No malloc.
 * Value semantics: elements are copied in/out (memcpy of elem_size bytes).
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli. */

typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;

typedef struct {
    uint8_t *buf;       /* storage, must be at least cap*elem_size bytes */
    uint16_t cap;       /* element capacity */
    uint16_t elem_size; /* bytes per element (1 = classic byte/UART buffer) */
    uint16_t head;      /* index of oldest element */
    uint16_t count;     /* elements currently stored */
    bool     overwrite; /* when full: evict oldest (true) vs reject (false) */
} ringbuf;

void     ringbuf_init(ringbuf *self, void *storage, uint16_t cap,
                      uint16_t elem_size, bool overwrite);
rb_err   ringbuf_put(ringbuf *self, const void *elem);
rb_err   ringbuf_get(ringbuf *self, void *out);   /* out may be NULL to just drop */
rb_err   ringbuf_peek(const ringbuf *self, void *out);
uint16_t ringbuf_count(const ringbuf *self);
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full(const ringbuf *self);
void     ringbuf_clear(ringbuf *self);

#endif /* CLIBS_RINGBUF_H */
