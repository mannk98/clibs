#ifndef CLIBS_HCSR04_DIST_H
#define CLIBS_HCSR04_DIST_H

#include <stdint.h>

/* Convert echo-high duration (us) to distance in mm: us * 343 / 2000 (speed of
 * sound ~343 m/s, round-trip halved). Valid echo <= ~38000 us (no uint32 overflow).
 * Pure: no SDK headers (host-testable). */
uint32_t hcsr04_us_to_mm(uint32_t echo_us);

#endif /* CLIBS_HCSR04_DIST_H */
