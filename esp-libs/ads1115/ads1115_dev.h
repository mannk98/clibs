#ifndef CLIBS_ADS1115_DEV_H
#define CLIBS_ADS1115_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "ads1115.h"

/* ADS1115 16-bit ADC over the shared i2c_bus (port 0). The bus must be brought
 * up first with i2c_bus_init(). SDK glue — compile-gated only; the pure
 * raw->microvolts conversion and config-register builder live in ads1115.{h,c}
 * (ads1115_to_microvolts / ads1115_config). */

/* The four possible 7-bit I2C addresses (ADDR pin -> GND / VDD / SDA / SCL). */
#define ADS1115_ADDR_GND 0x48
#define ADS1115_ADDR_VDD 0x49
#define ADS1115_ADDR_SDA 0x4A
#define ADS1115_ADDR_SCL 0x4B

/* ADS1115 register pointers. */
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01

typedef struct {
    uint8_t     addr;  /* 7-bit I2C address */
    ads1115_pga pga;   /* full-scale-range / gain setting */
} ads1115;

/* Store the address + PGA for subsequent reads. ESP_OK / ESP_ERR_INVALID_ARG
 * (NULL self). No bus traffic — single-shot config is written per read. */
esp_err_t ads1115_init(ads1115 *self, uint8_t addr, ads1115_pga pga);

/* Single-shot single-ended read of `channel` (0..3):
 *   1. write ads1115_config(channel, pga) to the config register (2B big-endian),
 *   2. wait for the conversion to be ready (~8 ms at 128 SPS, first-cut fixed),
 *   3. read the conversion register (2B big-endian) -> int16 -> microvolts via
 *      ads1115_to_microvolts.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ads1115_read(ads1115 *self, uint8_t channel, int32_t *microvolts);

#endif /* CLIBS_ADS1115_DEV_H */
