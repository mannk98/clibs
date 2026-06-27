#include "max7219_encode.h"

void max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2]) {
    if (!out) {
        return;
    }
    out[0] = reg;
    out[1] = data;
}

uint8_t max7219_digit_segments(uint8_t value, bool dp) {
    static const uint8_t SEG[10] = {
        0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B
    };
    uint8_t s = (value <= 9u) ? SEG[value] : 0x00u;
    if (dp) {
        s |= 0x80u;
    }
    return s;
}
