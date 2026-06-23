#include "throttle.h"
#include <stddef.h>

void throttle_init(throttle *self, uint32_t interval_ms)
{
    if (self == NULL) {
        return;
    }
    self->interval_ms = interval_ms;
    self->last_ms = 0;
    self->primed = false;
}

bool throttle_allow(throttle *self, uint32_t now_ms)
{
    if (self == NULL) {
        return false;
    }
    if (!self->primed) {
        self->primed = true;
        self->last_ms = now_ms;
        return true;
    }
    if ((uint32_t) (now_ms - self->last_ms) >= self->interval_ms) {
        self->last_ms = now_ms;
        return true;
    }
    return false;
}
