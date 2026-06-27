#ifndef CLIBS_DS3231_H
#define CLIBS_DS3231_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   /* NULL */

/* Pure (freestanding, host-testable) part of the DS3231 RTC driver.
 * No SDK headers here — the hardware glue over i2c_bus lives in
 * ds3231_dev.{h,c} and is COMPILE_GATE-only / deferred. */

/* Decoded wall-clock time, 24-hour mode. Fields are plain binary (not BCD):
 *   sec   0..59
 *   min   0..59
 *   hour  0..23   (24-hour mode)
 *   dow   1..7    (day-of-week, device-defined)
 *   date  1..31
 *   month 1..12
 *   year  full year, e.g. 2026 (= 2000 + the century-less RTC register) */
typedef struct {
    uint8_t  sec;
    uint8_t  min;
    uint8_t  hour;
    uint8_t  dow;
    uint8_t  date;
    uint8_t  month;
    uint16_t year;
} ds3231_time;

/* Decode the 7 timekeeping registers (addresses 0x00..0x06) into *out.
 * 24-hour BCD layout:
 *   sec   = bcd(raw[0] & 0x7F)
 *   min   = bcd(raw[1] & 0x7F)
 *   hour  = bcd(raw[2] & 0x3F)   (24-hour mode: ignore 12/24 + AM/PM bits)
 *   dow   = raw[3] & 0x07
 *   date  = bcd(raw[4] & 0x3F)
 *   month = bcd(raw[5] & 0x1F)   (ignore the century bit)
 *   year  = 2000 + bcd(raw[6])
 * Returns false and writes nothing if raw or out is NULL; otherwise true. */
bool ds3231_decode(const uint8_t raw[7], ds3231_time *out);

/* Encode *t back into the 7 timekeeping registers (inverse of ds3231_decode).
 * year is written as bcd(t->year % 100); all other fields are bcd of the
 * corresponding field. Round-trips ds3231_decode exactly for in-range input.
 * Returns false and writes nothing if t or raw is NULL; otherwise true. */
bool ds3231_encode(const ds3231_time *t, uint8_t raw[7]);

/* Convert the 2 on-chip temperature registers (0x11 MSB, 0x12 LSB) to
 * milli-degrees Celsius.
 *
 *   = (int32_t)(int8_t)msb * 1000 + (int32_t)(lsb >> 6) * 250
 *
 * The MSB is a signed 8-bit integer-degrees value: the sign is recovered with
 * an (int8_t) cast, NOT a left-shift of a possibly-negative value, so the
 * conversion is clean under UBSan. The LSB's top 2 bits give the 0.25 degC
 * fraction (0..3 -> 0,250,500,750 mC) and shift only a non-negative uint8. */
int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb);

#endif /* CLIBS_DS3231_H */
