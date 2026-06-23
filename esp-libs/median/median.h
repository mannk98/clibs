#ifndef CLIBS_MEDIAN_H
#define CLIBS_MEDIAN_H

#include <stdint.h>

/* Median of three values (despike). Apply over the last 3 samples of a stream. */
int32_t median3(int32_t a, int32_t b, int32_t c);

#endif /* CLIBS_MEDIAN_H */
