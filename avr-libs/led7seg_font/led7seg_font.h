#ifndef CLIBS_LED7SEG_FONT_H
#define CLIBS_LED7SEG_FONT_H

#include <stdint.h>

/* 7-segment font for a common-anode display driven through a 74HC595.
 * Returns the segment byte for a single decimal digit 0-9.
 * digit > 9 -> 0xFF (all segments off). */
uint8_t led7seg_segments(uint8_t digit);

#endif /* CLIBS_LED7SEG_FONT_H */
