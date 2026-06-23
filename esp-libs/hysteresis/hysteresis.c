#include "hysteresis.h"

void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial)
{
    if (self == NULL) {
        return;
    }
    if (low > high) {
        int32_t tmp = low;
        low = high;
        high = tmp;
    }
    self->low = low;
    self->high = high;
    self->on = initial;
}

bool hysteresis_update(hysteresis *self, int32_t value)
{
    if (self == NULL) {
        return false;
    }
    if (value >= self->high) {
        self->on = true;
    } else if (value <= self->low) {
        self->on = false;
    }
    return self->on;
}

bool hysteresis_state(const hysteresis *self)
{
    return (self != NULL) && self->on;
}
