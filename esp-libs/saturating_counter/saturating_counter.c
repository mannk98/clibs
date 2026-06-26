#include "saturating_counter.h"

void saturating_counter_init(saturating_counter *self, int32_t min, int32_t max, int32_t initial)
{
    if (self == NULL) {
        return;
    }
    if (min > max) {
        int32_t tmp = min;
        min = max;
        max = tmp;
    }
    if (initial < min) {
        initial = min;
    } else if (initial > max) {
        initial = max;
    }
    self->min = min;
    self->max = max;
    self->value = initial;
}

int32_t saturating_counter_inc(saturating_counter *self)
{
    if (self == NULL) {
        return 0;
    }
    /* Guard before incrementing: prevents signed overflow at max == INT32_MAX. */
    if (self->value < self->max) {
        self->value++;
    }
    return self->value;
}

int32_t saturating_counter_dec(saturating_counter *self)
{
    if (self == NULL) {
        return 0;
    }
    /* Guard before decrementing: prevents signed underflow at min == INT32_MIN. */
    if (self->value > self->min) {
        self->value--;
    }
    return self->value;
}

int32_t saturating_counter_get(const saturating_counter *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->value;
}