#ifndef CLIBS_INA219_H
#define CLIBS_INA219_H

#include <stdint.h>

/* Pure (freestanding, host-testable) part of the INA219 current/power-monitor
 * driver: the register raw -> physical-unit conversions and the calibration
 * register value. No SDK headers here — the I2C hardware glue over i2c_bus lives
 * in ina219_dev.{h,c} and is COMPILE_GATE-only / deferred. */

/* Bus voltage in millivolts from the bus-voltage register (0x02).
 * The low 3 bits are status flags (CNVR/OVF), so they are dropped, and each LSB
 * of the remaining value is 4 mV: mv = (raw >> 3) * 4.
 * Shift is on a uint16 (non-negative) only — no UB. */
int32_t ina219_bus_mv(uint16_t raw);

/* Shunt voltage in microvolts from the shunt-voltage register (0x01).
 * Signed, 10 uV per LSB: uv = raw * 10. */
int32_t ina219_shunt_uv(int16_t raw);

/* Current in microamps from the current register (0x04).
 * Signed raw scaled by the programmed current LSB (uA):
 * ua = raw * current_lsb_ua. Widened through int64 to avoid overflow. */
int32_t ina219_current_ua(int16_t raw, uint32_t current_lsb_ua);

/* Power in microwatts from the power register (0x03).
 * Unsigned raw; the INA219 power LSB is 20x the current LSB:
 * uw = raw * 20 * current_lsb_ua. Widened through int64 to avoid overflow. */
int32_t ina219_power_uw(uint16_t raw, uint32_t current_lsb_ua);

/* Value for the calibration register (0x05) from the chosen current LSB (uA) and
 * the shunt resistance (milliohm). Datasheet: cal = 0.04096 / (Current_LSB * Rshunt);
 * with Current_LSB in amps and Rshunt in ohms this is 40960000 / (current_lsb_ua *
 * shunt_milliohm). Returns 0 when the denominator is 0 (no divide-by-zero). */
uint16_t ina219_calibration(uint32_t current_lsb_ua, uint32_t shunt_milliohm);

#endif /* CLIBS_INA219_H */
