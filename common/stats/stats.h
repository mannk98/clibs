#ifndef CLIBS_STATS_H
#define CLIBS_STATS_H

/* stats — streaming min/max/mean/variance via Welford's online algorithm.
 *
 * Small malloc-free value object: the caller owns a `stats` on the stack,
 * constructs it with stats_init(), then feeds samples with stats_add().
 * mean/variance are kept incrementally (Welford) and reported as `float`
 * to match MCU soft-float and Unity's float asserts.
 *
 * Variance is the POPULATION variance (m2/count), NOT the sample variance.
 * No stats_stddev() is provided (would pull in <math.h>); a caller who needs
 * the standard deviation can sqrtf(stats_variance(&s)) itself. Freestanding:
 * depends only on <stdint.h> and <stddef.h>. Every function is NULL-self safe.
 */

#include <stdint.h>
#include <stddef.h>

typedef struct {                 /* fields private */
    size_t  count;
    int32_t min;
    int32_t max;
    float   mean;
    float   m2;                  /* sum of squared deviations (Welford) */
} stats;

/* Reset to the empty state: count 0, min/max 0, mean/variance 0.0f. */
void    stats_init(stats *self);

/* Fold one sample into the running statistics (Welford update).
 * count++; d = x - mean; mean += d/count; m2 += d*(x - mean). No-op if self NULL. */
void    stats_add(stats *self, int32_t x);

/* Number of samples seen. 0 if self NULL. */
size_t  stats_count(const stats *self);

/* Smallest sample seen. 0 if count==0 or self NULL. */
int32_t stats_min(const stats *self);

/* Largest sample seen. 0 if count==0 or self NULL. */
int32_t stats_max(const stats *self);

/* Running mean. 0.0f if count==0 or self NULL. */
float   stats_mean(const stats *self);

/* Population variance (m2/count). 0.0f if count==0 or self NULL. */
float   stats_variance(const stats *self);

#endif /* CLIBS_STATS_H */
