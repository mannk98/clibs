#ifndef CLIBS_ENCODER_DECODE_H
#define CLIBS_ENCODER_DECODE_H

#include <stdint.h>

/* Quadrature decode for one poll. a/b are the just-read A/B pin states.
 * Tracks the previous A state (*pre_a) and a half-detent counter (*count);
 * on every 2nd A-edge (a full detent) applies +1 (a==b) or -1 (a!=b) to *counter. */
void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter);

#endif /* CLIBS_ENCODER_DECODE_H */
