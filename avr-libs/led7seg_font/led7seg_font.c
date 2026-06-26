#include "led7seg_font.h"

/* bit-identical to the original B-macro table (B11000000 == 0xC0, ...) */
static const uint8_t LED7SEG_FONT[10] = {
    0xC0, /* 0 */
    0xCF, /* 1 */
    0xA4, /* 2 */
    0xB0, /* 3 */
    0x99, /* 4 */
    0x92, /* 5 */
    0x82, /* 6 */
    0xF8, /* 7 */
    0x80, /* 8 */
    0x90, /* 9 */
};

uint8_t led7seg_segments(uint8_t digit) {
    if (digit > 9)
        return 0xFF; /* all segments off */
    return LED7SEG_FONT[digit];
}
