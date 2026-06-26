#ifndef CLIBS_FIXED_H
#define CLIBS_FIXED_H

/* fixed — Q16.16 fixed-point value type (no FPU required on-target).
 *
 * `fixed_t` is a value type (like `int`): operations act on values, there is no
 * `self` receiver. 16 integer bits + 16 fractional bits in an int32_t.
 * Freestanding: depends only on <stdint.h>.
 *
 * Not an object — no init/deinit; just construct values and operate on them.
 */

#include <stdint.h>

/** Q16.16 fixed-point value: 16 integer bits, 16 fractional bits. */
typedef int32_t fixed_t;

/** Number of fractional bits in the Q16.16 representation. */
#define FIXED_SHIFT 16

/** The value 1.0 expressed in Q16.16 (65536). */
#define FIXED_ONE   (1L << FIXED_SHIFT)

/** Construct a fixed_t from an integer (v << 16). */
fixed_t fixed_from_int(int32_t v);

/** Integer part of a fixed_t, truncated toward zero (matches an (int) cast). */
int32_t fixed_to_int(fixed_t v);

/** Construct a fixed_t from a float (rounded; host/dev convenience). */
fixed_t fixed_from_float(float v);

/** Convert a fixed_t back to a float (host/dev convenience). */
float   fixed_to_float(fixed_t v);

/** Add two fixed_t values (a + b). */
fixed_t fixed_add(fixed_t a, fixed_t b);

/** Subtract two fixed_t values (a - b). */
fixed_t fixed_sub(fixed_t a, fixed_t b);

/** Multiply two fixed_t values; uses an int64_t intermediate. */
fixed_t fixed_mul(fixed_t a, fixed_t b);

/** Divide a by b; uses an int64_t intermediate. b == 0 returns 0 (documented). */
fixed_t fixed_div(fixed_t a, fixed_t b);

/** Square root of a fixed_t; v <= 0 returns 0. */
fixed_t fixed_sqrt(fixed_t v);

#endif /* CLIBS_FIXED_H */
