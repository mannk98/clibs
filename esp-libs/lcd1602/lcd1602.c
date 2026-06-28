#include "lcd1602.h"

void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4])
{
    if (!out) {
        return;
    }
    /* value is uint8_t; int-promoted to a positive int before the shift, so
     * (value << 4) never left-shifts a negative value (no UB). Masking to 0xF0
     * keeps only the high nibble of each presented half. */
    uint8_t hi = (uint8_t)(value & 0xF0u);
    uint8_t lo = (uint8_t)((value << 4) & 0xF0u);
    uint8_t base = (uint8_t)((rs ? 0x01u : 0u) | (backlight ? 0x08u : 0u));
    const uint8_t en = 0x04u;

    out[0] = (uint8_t)(hi | base | en); /* high nibble, EN high (latch-high) */
    out[1] = (uint8_t)(hi | base);      /* high nibble, EN low  (latch-low)  */
    out[2] = (uint8_t)(lo | base | en); /* low  nibble, EN high (latch-high) */
    out[3] = (uint8_t)(lo | base);      /* low  nibble, EN low  (latch-low)  */
}
