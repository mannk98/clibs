/* fixed — Q16.16 fixed-point value type. See fixed.h for the contract.
 *
 * mul/div use an int64_t intermediate so they don't overflow within the
 * Q16.16 range. The float helpers are host/dev convenience.
 */
#include "fixed.h"

/** Construct a fixed_t from an integer (v << 16). */
fixed_t fixed_from_int(int32_t v) {
    return (fixed_t)((int64_t)v << FIXED_SHIFT);
}

/** Integer part of a fixed_t, truncated toward zero (matches an (int) cast). */
int32_t fixed_to_int(fixed_t v) {
    if (v >= 0) return (int32_t)(v >> FIXED_SHIFT);
    return -(int32_t)((-(int64_t)v) >> FIXED_SHIFT);
}

/** Construct a fixed_t from a float (rounded to nearest). */
fixed_t fixed_from_float(float v) {
    float scaled = v * (float)FIXED_ONE;
    return (fixed_t)(scaled + (v >= 0.0f ? 0.5f : -0.5f));
}

/** Convert a fixed_t back to a float. */
float fixed_to_float(fixed_t v) {
    return (float)v / (float)FIXED_ONE;
}

/** Add two fixed_t values (a + b). */
fixed_t fixed_add(fixed_t a, fixed_t b) {
    return a + b;
}

/** Subtract two fixed_t values (a - b). */
fixed_t fixed_sub(fixed_t a, fixed_t b) {
    return a - b;
}

/** Multiply two fixed_t values; uses an int64_t intermediate. */
fixed_t fixed_mul(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FIXED_SHIFT);
}

/** Divide a by b; uses an int64_t intermediate. b == 0 returns 0. */
fixed_t fixed_div(fixed_t a, fixed_t b) {
    if (b == 0) return 0;
    return (fixed_t)(((int64_t)a << FIXED_SHIFT) / (int64_t)b);
}

/** Square root of a fixed_t (integer Newton's method); v <= 0 returns 0. */
fixed_t fixed_sqrt(fixed_t v) {
    if (v <= 0) return 0;
    uint64_t n = (uint64_t)v << FIXED_SHIFT;       /* result = isqrt(v << 16) */
    uint64_t x = n, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }  /* integer Newton's method */
    return (fixed_t)x;
}
