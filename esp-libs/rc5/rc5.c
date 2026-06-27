#include "rc5.h"

#define RC5_BITS      14
#define RC5_HALFBITS  28

bool rc5_decode(const uint8_t *halfbits, size_t count, rc5_result *out)
{
    if (!halfbits || !out) {
        return false;
    }
    if (count != RC5_HALFBITS) {
        return false;
    }

    uint16_t raw = 0;
    for (size_t i = 0; i < RC5_BITS; i++) {
        uint8_t first  = halfbits[2u * i];
        uint8_t second = halfbits[2u * i + 1u];
        uint8_t bit;
        if (first == 0u && second == 1u) {        /* (0,1) -> logical 1 */
            bit = 1u;
        } else if (first == 1u && second == 0u) { /* (1,0) -> logical 0 */
            bit = 0u;
        } else {                                  /* invalid Manchester pair */
            return false;
        }
        raw = (uint16_t)((uint16_t)(raw << 1) | bit);
    }

    /* Validate the leading start bit S1 (bit 13). */
    if (((raw >> 13) & 1u) == 0u) {
        return false;
    }

    out->toggle  = (uint8_t)((raw >> 11) & 1u);
    out->address = (uint8_t)((raw >> 6) & 0x1Fu);
    out->command = (uint8_t)(raw & 0x3Fu);
    out->raw     = raw;
    return true;
}
