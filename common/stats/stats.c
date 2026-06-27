/* stats — streaming min/max/mean/variance via Welford's online algorithm.
 *
 * Population variance (m2/count), reported as float to match MCU soft-float
 * and Unity's float asserts. Freestanding: depends only on the public header.
 * Every function is NULL-self safe. No malloc — state lives in the caller's
 * stack-owned `stats` object.
 */

#include "stats.h"

void stats_init(stats *self) {
    if (self == NULL) {
        return;
    }
    self->count = 0;
    self->min = 0;
    self->max = 0;
    self->mean = 0.0f;
    self->m2 = 0.0f;
}

void stats_add(stats *self, int32_t x) {
    if (self == NULL) {
        return;
    }
    if (self->count == 0) {
        self->min = x;
        self->max = x;
    } else {
        if (x < self->min) {
            self->min = x;
        }
        if (x > self->max) {
            self->max = x;
        }
    }
    /* Welford update: count++; d = x-mean; mean += d/count; m2 += d*(x-mean). */
    self->count++;
    float d = (float)x - self->mean;
    self->mean += d / (float)self->count;
    self->m2 += d * ((float)x - self->mean);
}

size_t stats_count(const stats *self) {
    if (self == NULL) {
        return 0;
    }
    return self->count;
}

int32_t stats_min(const stats *self) {
    if (self == NULL || self->count == 0) {
        return 0;
    }
    return self->min;
}

int32_t stats_max(const stats *self) {
    if (self == NULL || self->count == 0) {
        return 0;
    }
    return self->max;
}

float stats_mean(const stats *self) {
    if (self == NULL || self->count == 0) {
        return 0.0f;
    }
    return self->mean;
}

float stats_variance(const stats *self) {
    if (self == NULL || self->count == 0) {
        return 0.0f;
    }
    return self->m2 / (float)self->count;
}
