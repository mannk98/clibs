#ifndef CLIBS_ADS1115_H
#define CLIBS_ADS1115_H

#include <stdint.h>

/* Pure (freestanding, host-testable) part of the ADS1115 16-bit I2C ADC driver.
 * No SDK headers here — the hardware glue over i2c_bus lives in ads1115_dev.{h,c}
 * and is COMPILE_GATE-only / deferred. */

/* Programmable-gain-amplifier full-scale-range settings. The enum value is the
 * 3-bit PGA field placed at bits [11:9] of the config register. */
typedef enum {
    ADS1115_PGA_6V144 = 0,  /* +/- 6.144 V */
    ADS1115_PGA_4V096,      /* +/- 4.096 V */
    ADS1115_PGA_2V048,      /* +/- 2.048 V (device default) */
    ADS1115_PGA_1V024,      /* +/- 1.024 V */
    ADS1115_PGA_0V512,      /* +/- 0.512 V */
    ADS1115_PGA_0V256       /* +/- 0.256 V */
} ads1115_pga;

/* Convert a signed 16-bit raw conversion result to microvolts for the given PGA.
 *
 *   microvolts = raw * FSR_uV[pga] / 32768
 *
 * where FSR_uV = {6144000,4096000,2048000,1024000,512000,256000}. The raw count
 * is widened to int64 BEFORE the multiply so that -32768 * 6144000 cannot
 * overflow 32 bits; the result truncates toward zero and fits int32. A `pga`
 * value outside the enum clamps to ADS1115_PGA_2V048 rather than indexing the
 * FSR table out of bounds. */
int32_t ads1115_to_microvolts(int16_t raw, ads1115_pga pga);

/* Build the 16-bit config-register value for a single-shot single-ended read.
 *
 *   0x8000                      OS = 1 (start single conversion)
 * | ((0x4 | (channel & 3)) << 12)  MUX = single-ended AINx vs GND
 * | (pga << 9)                 PGA gain field
 * | 0x0100                     MODE = 1 (single-shot)
 * | 0x0080                     DR  = 128 SPS
 * | 0x0003                     COMP_QUE = disable comparator
 *
 * `channel` is masked to 0..3; `pga` is used verbatim in bits [11:9]. */
uint16_t ads1115_config(uint8_t channel, ads1115_pga pga);

#endif /* CLIBS_ADS1115_H */
