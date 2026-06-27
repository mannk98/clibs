#ifndef CLIBS_HX711_H
#define CLIBS_HX711_H

#include <stdint.h>

/* Pure part of the HX711 24-bit load-cell ADC driver (host-testable).
 * No SDK headers. The hardware bit-bang glue lives in hx711_gpio.{h,c}. */

/* Parse 3 raw bytes (big-endian, raw[0]=MSB) as a 24-bit two's-complement
 * value, sign-extended to int32_t. NULL -> 0.
 *
 * Sign-extension uses an unsigned OR-mask then a cast to int32_t — never a
 * shift of a signed/negative value — so the conversion is UB-clean. */
int32_t hx711_parse(const uint8_t raw[3]);

#endif /* CLIBS_HX711_H */
