#ifndef CLIBS_FILTER_H
#define CLIBS_FILTER_H

#include <stdint.h>

/* Integer exponential moving average. alpha = 1/2^shift. `acc` is the value in
 * fixed-point (value << shift). Keep |value| << shift within int32 range
 * (fine for ADC 0..1023 with shift <= 8). Not thread-safe. */
typedef struct {
    int32_t acc;
    uint8_t shift;
} ema;

void    ema_init(ema *self, uint8_t shift, int32_t initial);
int32_t ema_update(ema *self, int32_t sample); /* returns the new smoothed value */

#endif /* CLIBS_FILTER_H */
