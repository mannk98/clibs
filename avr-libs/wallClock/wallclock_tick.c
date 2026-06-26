#include "wallclock_tick.h"

/* microseconds per Timer0 overflow = (64 prescaler * 256 counts) / (F_CPU MHz). */
#define MICROSECONDS_PER_TIMER0_OVERFLOW (64 * 256 / (F_CPU / 1000000L))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)        /* 16MHz -> 1 */
#define FRACT_INC  ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3) /* 16MHz -> 24>>3 = 3 */
#define FRACT_MAX  (1000 >> 3)                                      /* 125 */

void wallclock_tick(wallclock_accum *a) {
    a->millis += MILLIS_INC;
    a->fract  += FRACT_INC;
    if (a->fract >= FRACT_MAX) {
        a->fract  -= FRACT_MAX;
        a->millis += 1;
    }
}
