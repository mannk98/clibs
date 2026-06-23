#include "median.h"

int32_t median3(int32_t a, int32_t b, int32_t c)
{
    int32_t tmp;
    if (a > b) { tmp = a; a = b; b = tmp; }   /* a <= b */
    if (b > c) { tmp = b; b = c; c = tmp; }   /* b <= c */
    if (a > b) { tmp = a; a = b; b = tmp; }   /* a <= b */
    return b;                                  /* the middle value */
}
