#ifndef CLIBS_STEPPER_H
#define CLIBS_STEPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  /* NULL */

/* 4-wire unipolar (28BYJ-48) half-step sequencer — pure, host-testable.
 * Coil pattern bits 0..3 map to IN1..IN4. Half-step table (8 phases):
 *   {0x01,0x03,0x02,0x06,0x04,0x0C,0x08,0x09}.
 * step(forward): phase = forward ? (phase+1)&7 : (phase+7)&7;
 *                position += forward ? 1 : -1; returns HALF[phase].
 * Fields are private; access only through the API. Not reentrant. */
typedef struct {
    uint8_t phase;     /* private: current index into the half-step table (0..7) */
    int32_t position;  /* private: signed cumulative half-step count */
} stepper;

/* Reset to phase 0 (coil pattern 0x01) and position 0. NULL self -> no-op. */
void stepper_init(stepper *self);

/* Advance one half-step in the given direction; position += dir.
 * Returns the new coil pattern (IN1..IN4 = bits 0..3); 0 if self is NULL. */
uint8_t stepper_step(stepper *self, bool forward);

/* Current coil pattern without advancing; 0 if self is NULL. */
uint8_t stepper_coils(const stepper *self);

/* Current signed half-step position; 0 if self is NULL. */
int32_t stepper_position(const stepper *self);

#endif /* CLIBS_STEPPER_H */
