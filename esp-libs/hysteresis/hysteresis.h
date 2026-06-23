#ifndef CLIBS_HYSTERESIS_H
#define CLIBS_HYSTERESIS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Rising-edge Schmitt trigger: output goes ON at value >= high, OFF at value <= low,
 * and holds in the (low, high) deadband. Not thread-safe. */
typedef struct {
    int32_t low;
    int32_t high;
    bool    on;
} hysteresis;

/* Init thresholds + initial output. If low > high, they are swapped. */
void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial);
/* Feed a value; returns the (possibly updated) output. */
bool hysteresis_update(hysteresis *self, int32_t value);
/* Current output (false if self is NULL). */
bool hysteresis_state(const hysteresis *self);

#endif /* CLIBS_HYSTERESIS_H */
