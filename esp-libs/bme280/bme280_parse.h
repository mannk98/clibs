#ifndef CLIBS_BME280_PARSE_H
#define CLIBS_BME280_PARSE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Pure part of the BME280 driver: trimming-coefficient parsing + Bosch integer
 * compensation. No SDK headers (host-testable). */

/* Factory trimming coefficients read out of the calib registers. */
typedef struct {
    uint16_t T1;
    int16_t  T2;
    int16_t  T3;
    uint16_t P1;
    int16_t  P2;
    int16_t  P3;
    int16_t  P4;
    int16_t  P5;
    int16_t  P6;
    int16_t  P7;
    int16_t  P8;
    int16_t  P9;
    uint8_t  H1;
    int16_t  H2;
    uint8_t  H3;
    int16_t  H4;
    int16_t  H5;
    int8_t   H6;
} bme280_calib;

/* Compensated reading in SI units. */
typedef struct {
    int32_t  temp_mc;    /* temperature, milli-degrees C        */
    uint32_t press_pa;   /* pressure, pascals                   */
    uint32_t humi_mrh;   /* relative humidity, milli-percent RH */
} bme280_reading;

/* Parse the two calib register blocks (calib26 = 0x88..0xA1, calibH = 0xE1..0xE7)
 * into the trimming coefficients. The packed 12-bit signed H4/H5 are sign-extended.
 * NULL on any pointer -> false. */
bool bme280_parse_calib(const uint8_t calib26[26], const uint8_t calibH[7],
                        bme280_calib *out);

/* Bosch integer compensation of the raw ADC readings, converted to SI units.
 * NULL cal or out -> false. */
bool bme280_compensate(const bme280_calib *cal, int32_t adc_T, int32_t adc_P,
                       int32_t adc_H, bme280_reading *out);

#endif /* CLIBS_BME280_PARSE_H */
