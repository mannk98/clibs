#ifndef CLIBS_ROTARY_H
#define CLIBS_ROTARY_H

#include <stdint.h>

/* Direction reported by one decode step. */
typedef enum { ROTARY_NONE, ROTARY_CW, ROTARY_CCW } rotary_dir;

/* Quadrature rotary-encoder decoder (quarter-step). Fields private. Not reentrant. */
typedef struct { uint8_t prev_ab; int32_t position; } rotary;

/* Initialise from the current A/B level. ab = (a<<1)|b (only the low 2 bits used);
 * position resets to 0. NULL self -> no-op. */
void       rotary_init(rotary *self, uint8_t ab);
/* Feed the new (a<<1)|b. A valid Gray transition counts +1 (CW) / -1 (CCW) into position and returns
 * ROTARY_CW/ROTARY_CCW; same state or a 2-step jump returns ROTARY_NONE (position unchanged). NULL self
 * -> ROTARY_NONE. */
rotary_dir rotary_update(rotary *self, uint8_t ab);
/* Current accumulated quarter-step count (~4 per detent). 0 if NULL. */
int32_t    rotary_position(const rotary *self);

#endif /* CLIBS_ROTARY_H */
