#include "rng.h"

#include <stddef.h> /* NULL */

void rng_seed(rng *self, uint32_t seed)
{
    if (self == NULL) {
        return;
    }
    /* xorshift32 cannot start from 0; coerce a 0 seed to 1. */
    self->state = (seed == 0u) ? 1u : seed;
}

uint32_t rng_next(rng *self)
{
    if (self == NULL) {
        return 0u;
    }
    /* xorshift32 (Marsaglia). All operands are uint32_t — no signed shift UB. */
    uint32_t x = self->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    self->state = x;
    return x;
}

uint32_t rng_below(rng *self, uint32_t bound)
{
    if (self == NULL || bound == 0u) {
        return 0u;
    }
    /* Plain modulo; modulo bias acceptable for a small PRNG (documented). */
    return rng_next(self) % bound;
}
