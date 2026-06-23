#include "map_range.h"

int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    if (in_min == in_max) {
        return out_min;
    }
    int64_t num = (int64_t) (x - in_min) * (int64_t) (out_max - out_min);
    return (int32_t) (num / (int64_t) (in_max - in_min) + out_min);
}

int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}
