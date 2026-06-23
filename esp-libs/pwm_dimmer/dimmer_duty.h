#ifndef CLIBS_DIMMER_DUTY_H
#define CLIBS_DIMMER_DUTY_H

#include <stdint.h>

/* Map percent (clamped to 0..100) to a duty in [0, max_duty], rounded to nearest.
 * Pure: no SDK headers (host-testable). */
uint32_t dimmer_duty(uint8_t percent, uint32_t max_duty);

#endif /* CLIBS_DIMMER_DUTY_H */
