#include "tm1637.h"

#include <stddef.h>   /* NULL */

uint8_t tm1637_digit_segments(uint8_t digit)
{
    static const uint8_t SEG[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };
    return (digit <= 9u) ? SEG[digit] : 0x00u;
}

void tm1637_encode_number(uint16_t value, bool leading_zeros, uint8_t out[4])
{
    if (out == NULL) {
        return;
    }

    uint16_t v = (uint16_t)(value % 10000u);

    uint8_t digits[4];
    digits[3] = (uint8_t)(v % 10u); v = (uint16_t)(v / 10u);   /* ones */
    digits[2] = (uint8_t)(v % 10u); v = (uint16_t)(v / 10u);   /* tens */
    digits[1] = (uint8_t)(v % 10u); v = (uint16_t)(v / 10u);   /* hundreds */
    digits[0] = (uint8_t)(v % 10u);                            /* thousands */

    bool seen_nonzero = false;
    for (uint8_t i = 0; i < 4u; i++) {
        bool is_ones = (i == 3u);
        if (digits[i] != 0u) {
            seen_nonzero = true;
        }
        /* Show the digit if: leading zeros requested, OR a non-zero digit has
         * been seen at or before this position, OR this is the ones digit. */
        if (leading_zeros || seen_nonzero || is_ones) {
            out[i] = tm1637_digit_segments(digits[i]);
        } else {
            out[i] = 0x00u;
        }
    }
}
