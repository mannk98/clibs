#ifndef CLIBS_MAX7219_ENCODE_H
#define CLIBS_MAX7219_ENCODE_H

#include <stdint.h>
#include <stdbool.h>

/* Build a 16-bit MAX7219 command (register addr + data) into 2 bytes, MSB-first:
 * out[0]=reg, out[1]=data. NULL out -> no-op. */
void max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2]);

/* Decimal digit 0..9 -> no-decode segment byte (bit7=DP, b6=A, b5=B, b4=C, b3=D,
 * b2=E, b1=F, b0=G). Any other value -> blank (0x00). dp ORs in bit7. */
uint8_t max7219_digit_segments(uint8_t value, bool dp);

#endif /* CLIBS_MAX7219_ENCODE_H */
