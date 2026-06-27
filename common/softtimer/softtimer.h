#ifndef CLIBS_SOFTTIMER_H
#define CLIBS_SOFTTIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  /* NULL */

/* softtimer — wrap-safe poll-based countdown timer.
 *
 * A small caller-allocated object (transparent struct, no malloc, no callbacks).
 * The caller advances an internal monotonic tick counter with softtimer_update()
 * and polls its return value to learn when the timer has fired. Two modes:
 *   - one-shot:  start(duration, false) — fires once, then stops.
 *   - auto-reload: start(duration, true) — fires repeatedly, reloading by one
 *     period each fire (at most one fire reported per update call).
 *
 * Expiry is compared with a signed difference, (int32_t)(now - expiry) >= 0, so
 * firing is correct even when `now` and `expiry` straddle a uint32_t rollover,
 * as long as the gap stays within ~2^31 ticks. Fields are private.
 */
typedef struct {
    uint32_t now;        /* internal monotonic tick counter */
    uint32_t expiry;     /* tick at which the timer is due */
    uint32_t period;     /* reload interval; 0 = one-shot */
    bool     running;    /* armed/active flag */
} softtimer;

/* Construct: clear all state (not running, remaining 0). NULL self is a no-op. */
void softtimer_init(softtimer *self);

/* Arm the timer relative to the current internal time:
 * expiry = now + duration; period = repeating ? duration : 0; running = true.
 * Intended for duration >= 1. NULL self is a no-op. */
void softtimer_start(softtimer *self, uint32_t duration, bool repeating);

/* Advance internal time by `elapsed` ticks. Returns true if the timer fired
 * during this call. For an auto-reload timer the expiry is bumped by one period
 * (one fire reported per update); for a one-shot timer the timer stops on fire.
 * Returns false (and does nothing) if self is NULL or the timer is not running. */
bool softtimer_update(softtimer *self, uint32_t elapsed);

/* True if the timer is currently armed. NULL self returns false. */
bool softtimer_running(const softtimer *self);

/* Disarm the timer: running = false. NULL self is a no-op. */
void softtimer_stop(softtimer *self);

/* Ticks remaining until expiry; 0 if not running or already due.
 * NULL self returns 0. */
uint32_t softtimer_remaining(const softtimer *self);

#endif /* CLIBS_SOFTTIMER_H */
