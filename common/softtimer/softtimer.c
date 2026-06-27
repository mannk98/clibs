#include "softtimer.h"

void softtimer_init(softtimer *self)
{
    if (self == NULL) {
        return;
    }
    self->now = 0u;
    self->expiry = 0u;
    self->period = 0u;
    self->running = false;
}

void softtimer_start(softtimer *self, uint32_t duration, bool repeating)
{
    if (self == NULL) {
        return;
    }
    self->expiry = self->now + duration;     /* relative to current now */
    self->period = repeating ? duration : 0u;
    self->running = true;
}

bool softtimer_update(softtimer *self, uint32_t elapsed)
{
    if (self == NULL) {
        return false;
    }

    self->now += elapsed;

    if (!self->running) {
        return false;
    }

    /* Wrap-safe due check: signed difference stays correct across a uint32_t
     * rollover as long as the gap is within ~2^31 ticks. */
    if ((int32_t)(self->now - self->expiry) >= 0) {
        if (self->period != 0u) {
            self->expiry += self->period;    /* auto-reload, one fire per update */
        } else {
            self->running = false;           /* one-shot done */
        }
        return true;
    }

    return false;
}

bool softtimer_running(const softtimer *self)
{
    if (self == NULL) {
        return false;
    }
    return self->running;
}

void softtimer_stop(softtimer *self)
{
    if (self == NULL) {
        return;
    }
    self->expiry = 0u;
    self->period = 0u;
    self->running = false;
}

uint32_t softtimer_remaining(const softtimer *self)
{
    if (self == NULL) {
        return 0u;
    }
    if (!self->running) {
        return 0u;
    }
    /* Already due -> 0; otherwise ticks until expiry (wrap-safe). */
    if ((int32_t)(self->now - self->expiry) >= 0) {
        return 0u;
    }
    return self->expiry - self->now;
}
