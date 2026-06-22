#include "backoff.h"
#include <stddef.h>

void backoff_init(backoff *self, uint32_t base_ms, uint32_t max_ms) {
    if (self == NULL) return;
    if (max_ms < base_ms) max_ms = base_ms;
    self->base_ms = base_ms;
    self->max_ms  = max_ms;
    self->cur_ms  = base_ms;
}

uint32_t backoff_next(backoff *self) {
    if (self == NULL) return 0;
    uint32_t d = self->cur_ms;
    self->cur_ms = (self->cur_ms <= self->max_ms / 2) ? (self->cur_ms * 2) : self->max_ms;
    return d;
}

void backoff_reset(backoff *self) {
    if (self == NULL) return;
    self->cur_ms = self->base_ms;
}
