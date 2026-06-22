#include "debounce.h"
#include <stddef.h>

void debounce_init(debounce *self, uint8_t threshold, uint8_t initial) {
    if (self == NULL) return;
    self->threshold = (threshold == 0) ? 1 : threshold;
    self->stable    = initial ? 1 : 0;
    self->candidate = self->stable;
    self->count     = 0;
}

debounce_edge debounce_update(debounce *self, uint8_t raw) {
    if (self == NULL) return DEBOUNCE_NONE;
    raw = raw ? 1 : 0;

    if (raw == self->stable) {       /* no pending change */
        self->count = 0;
        return DEBOUNCE_NONE;
    }
    if (raw == self->candidate) {
        if (self->count < 0xFF) self->count++;
    } else {
        self->candidate = raw;
        self->count = 1;
    }
    if (self->count >= self->threshold) {
        self->stable = raw;
        self->count = 0;
        return raw ? DEBOUNCE_RISING : DEBOUNCE_FALLING;
    }
    return DEBOUNCE_NONE;
}

uint8_t debounce_level(const debounce *self) {
    return self ? self->stable : 0;
}
