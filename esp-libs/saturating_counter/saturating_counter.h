#ifndef CLIBS_SATURATING_COUNTER_H
#define CLIBS_SATURATING_COUNTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Bounded (saturating) counter over a closed integer interval [min, max].
 * inc/dec never wrap: they clamp at the bounds. Caller owns storage
 * (transparent struct, no malloc). Not thread-safe. */
typedef struct {
    int32_t min;
    int32_t max;
    int32_t value;
} saturating_counter;

/* Initialize bounds + value. If min>max the bounds are swapped so min<=max
 * always holds. `initial` is clamped into [min,max]. NULL self is a no-op. */
void saturating_counter_init(saturating_counter *self, int32_t min, int32_t max, int32_t initial);

/* Increment by 1, saturating at max. Returns the new current value.
 * NULL self returns 0. Already at max -> stays at max. */
int32_t saturating_counter_inc(saturating_counter *self);

/* Decrement by 1, saturating at min. Returns the new current value.
 * NULL self returns 0. Already at min -> stays at min. */
int32_t saturating_counter_dec(saturating_counter *self);

/* Read the current value without mutating. NULL self returns 0. */
int32_t saturating_counter_get(const saturating_counter *self);

#endif /* CLIBS_SATURATING_COUNTER_H */