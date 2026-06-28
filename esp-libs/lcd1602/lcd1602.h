#ifndef CLIBS_LCD1602_H
#define CLIBS_LCD1602_H

#include <stdint.h>
#include <stdbool.h>

/* Pure PCF8574 4-bit nibble framing for an HD44780 char-LCD behind a PCF8574
 * I2C backpack. Freestanding (no SDK). Host-testable. Not reentrant only in the
 * trivial sense — this is a pure function with no shared state. SDK glue lives
 * in lcd1602_dev.{h,c}.
 *
 * PCF8574 wiring: P0=RS, P1=RW(=0), P2=EN, P3=backlight, P4..P7=D4..D7. */

/* Clock ONE HD44780 byte in 4-bit mode into four PCF8574 bytes written in order.
 * hi = value & 0xF0; lo = (value << 4) & 0xF0;
 * base = (rs?0x01:0) | (backlight?0x08:0); EN = 0x04.
 * out = { hi|base|EN, hi|base, lo|base|EN, lo|base } — i.e. each nibble is
 * presented with EN high (latch-high) then EN low (latch-low). rs selects the
 * register (true=data RS=1, false=command RS=0); backlight drives P3.
 * NULL out -> no-op. */
void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4]);

#endif /* CLIBS_LCD1602_H */
