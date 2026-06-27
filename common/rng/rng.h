#ifndef CLIBS_RNG_H
#define CLIBS_RNG_H

#include <stdint.h>

/* rng — xorshift32 pseudo-random number generator (small stateful object).
 *
 * Deterministic and reproducible from a seed. Caller allocates the struct on
 * the stack; the library owns no heap memory. Every method is NULL-self-safe.
 *
 * The `state` field is private: callers seed/advance it only via the methods.
 */
typedef struct {
    uint32_t state; /* private: current xorshift32 state (never 0) */
} rng;

/* Seed the generator. A seed of 0 is coerced to 1 because xorshift32 cannot
 * start from state 0 (it would be stuck at 0 forever). No-op if self is NULL. */
void rng_seed(rng *self, uint32_t seed);

/* Advance the generator and return the next 32-bit value using the xorshift32
 * step (x ^= x<<13; x ^= x>>17; x ^= x<<5) — all on uint32_t, so no signed/
 * negative left-shift UB. Returns 0 if self is NULL. */
uint32_t rng_next(rng *self);

/* Return a value in [0, bound) via rng_next() % bound. A bound of 0 returns 0.
 * Uses plain modulo: the slight modulo bias is acceptable for a small PRNG.
 * Returns 0 if self is NULL. */
uint32_t rng_below(rng *self, uint32_t bound);

#endif /* CLIBS_RNG_H */
