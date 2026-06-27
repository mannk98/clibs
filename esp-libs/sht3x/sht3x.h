#ifndef CLIBS_SHT3X_H
#define CLIBS_SHT3X_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   /* NULL */

/* Pure (freestanding, host-testable) part of the SHT3x temperature/humidity
 * driver. No SDK headers here — the hardware glue over i2c_bus lives in
 * sht3x_dev.{h,c} and is COMPILE_GATE-only. */

/* Parse a single-shot SHT3x measurement frame into milli-degrees-Celsius and
 * milli-percent-relative-humidity.
 *
 * raw is the 6-byte frame [t_msb, t_lsb, t_crc, h_msb, h_lsb, h_crc].
 * Each 16-bit word is protected by a Sensirion CRC8 (polynomial 0x31,
 * init 0xFF, MSB-first, no final XOR): t_crc covers raw[0..1] and h_crc
 * covers raw[3..4]. Any CRC mismatch -> false (nothing written).
 *
 *   raw_t   = ((uint32_t)t_msb << 8) | t_lsb
 *   raw_h   = ((uint32_t)h_msb << 8) | h_lsb
 *   *temp_mc  = -45000 + (int32_t)((int64_t)175000 * raw_t / 65535)
 *   *humi_mrh =          (int32_t)((int64_t)100000 * raw_h / 65535)
 *
 * The multiply widens to int64 before multiplying (no 32-bit overflow at the
 * 0xFFFF ceiling) and the CRC loop shifts only non-negative uint8 values
 * (no signed left-shift UB).
 *
 * Returns false and writes nothing if raw, temp_mc, or humi_mrh is NULL, or if
 * either CRC fails; otherwise returns true. */
bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh);

#endif /* CLIBS_SHT3X_H */
