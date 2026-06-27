#include "hx711.h"

int32_t hx711_parse(const uint8_t raw[3])
{
    if (!raw) {
        return 0;
    }

    /* Assemble the 24-bit big-endian magnitude into an unsigned word.
     * All operands are non-negative, so each shift is well-defined. */
    uint32_t v = ((uint32_t)raw[0] << 16)
               | ((uint32_t)raw[1] << 8)
               | (uint32_t)raw[2];

    /* Sign-extend bit 23 into the high byte via an unsigned OR-mask, then
     * reinterpret as signed — no shift of a negative value, no UB. */
    if (v & 0x800000u) {
        v |= 0xFF000000u;
    }

    return (int32_t)v;
}
