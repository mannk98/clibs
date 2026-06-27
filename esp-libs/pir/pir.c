#include "pir.h"

void pir_init(pir *self, uint32_t hold_ms) {
    if (!self) {
        return;
    }
    self->hold_ms = hold_ms;
    self->last_ms = 0;
    self->active = false;
}

pir_event pir_update(pir *self, bool motion_raw, uint32_t now_ms) {
    if (!self) {
        return PIR_NONE;
    }
    if (motion_raw) {
        self->last_ms = now_ms;
        if (!self->active) {
            self->active = true;
            return PIR_MOTION_START;
        }
        return PIR_NONE;
    }
    /* Wrap-safe unsigned subtraction: (now - last) is well-defined on uint32_t. */
    if (self->active && (uint32_t)(now_ms - self->last_ms) >= self->hold_ms) {
        self->active = false;
        return PIR_MOTION_END;
    }
    return PIR_NONE;
}

bool pir_active(const pir *self) {
    return self ? self->active : false;
}
