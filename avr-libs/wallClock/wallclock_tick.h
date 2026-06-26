#ifndef CLIBS_WALLCLOCK_TICK_H
#define CLIBS_WALLCLOCK_TICK_H

#include <stdint.h>

/* Millisecond accumulator advanced once per Timer0 overflow. */
typedef struct {
    uint32_t millis;
    uint8_t  fract;
} wallclock_accum;

/* Advance the accumulator by one Timer0 overflow. Requires F_CPU to be defined
 * at compile time (e.g. -DF_CPU=16000000UL). */
void wallclock_tick(wallclock_accum *a);

#endif /* CLIBS_WALLCLOCK_TICK_H */
