#ifndef CLIBS_MAP_RANGE_H
#define CLIBS_MAP_RANGE_H

#include <stdint.h>

/* Linear map of x from [in_min,in_max] to [out_min,out_max] (no clamping; int64
 * intermediate so it is overflow-safe for int32 inputs). in_min==in_max -> out_min. */
int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
/* Clamp x to [lo,hi] (assumes lo <= hi). */
int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi);

#endif /* CLIBS_MAP_RANGE_H */
