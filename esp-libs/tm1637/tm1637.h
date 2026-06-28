#ifndef CLIBS_TM1637_H
#define CLIBS_TM1637_H

#include <stdint.h>
#include <stdbool.h>

/* Pure 7-segment encode for a 4-digit TM1637 display. Freestanding (no SDK).
 * Host-testable. Not reentrant only in the trivial sense — these are pure
 * functions with no shared state. SDK glue lives in tm1637_dev.{h,c}. */

/* Decimal digit 0..9 -> common-cathode segment byte (bit0=a, bit1=b, bit2=c,
 * bit3=d, bit4=e, bit5=f, bit6=g; bit7 unused). Any value >9 -> blank (0x00).
 * Table: 0->0x3F 1->0x06 2->0x5B 3->0x4F 4->0x66 5->0x6D 6->0x7D 7->0x07
 *        8->0x7F 9->0x6F. */
uint8_t tm1637_digit_segments(uint8_t digit);

/* Render value%10000 across out[0..3], out[0]=thousands .. out[3]=ones, each as
 * a segment byte from tm1637_digit_segments. leading_zeros=false blanks (0x00)
 * leading zero digits, but the ones digit is always shown; leading_zeros=true
 * shows every digit including leading zeros. NULL out -> no-op. */
void tm1637_encode_number(uint16_t value, bool leading_zeros, uint8_t out[4]);

#endif /* CLIBS_TM1637_H */
