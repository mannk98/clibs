#include "ds3231.h"

/* Packed BCD (two decimal digits per byte) <-> binary helpers. */
static uint8_t bcd_to_bin(uint8_t v)
{
    return (uint8_t)((v >> 4) * 10u + (v & 0x0Fu));
}

static uint8_t bin_to_bcd(uint8_t v)
{
    return (uint8_t)(((v / 10u) << 4) | (v % 10u));
}

bool ds3231_decode(const uint8_t raw[7], ds3231_time *out)
{
    if (raw == NULL || out == NULL) {
        return false;
    }

    out->sec   = bcd_to_bin((uint8_t)(raw[0] & 0x7Fu));
    out->min   = bcd_to_bin((uint8_t)(raw[1] & 0x7Fu));
    out->hour  = bcd_to_bin((uint8_t)(raw[2] & 0x3Fu));
    out->dow   = (uint8_t)(raw[3] & 0x07u);
    out->date  = bcd_to_bin((uint8_t)(raw[4] & 0x3Fu));
    out->month = bcd_to_bin((uint8_t)(raw[5] & 0x1Fu));
    out->year  = (uint16_t)(2000u + bcd_to_bin(raw[6]));
    return true;
}

bool ds3231_encode(const ds3231_time *t, uint8_t raw[7])
{
    if (t == NULL || raw == NULL) {
        return false;
    }

    raw[0] = bin_to_bcd(t->sec);
    raw[1] = bin_to_bcd(t->min);
    raw[2] = bin_to_bcd(t->hour);
    raw[3] = (uint8_t)(t->dow & 0x07u);
    raw[4] = bin_to_bcd(t->date);
    raw[5] = bin_to_bcd(t->month);
    raw[6] = bin_to_bcd((uint8_t)(t->year % 100u));
    return true;
}

int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb)
{
    /* Sign of the integer-degrees byte comes from the (int8_t) cast, not a
     * left-shift of a negative value. The fraction shifts only a uint8. */
    return (int32_t)(int8_t)msb * 1000 + (int32_t)(lsb >> 6) * 250;
}
