#ifndef CLIBS_BME280_H
#define CLIBS_BME280_H

#include <stdint.h>
#include "esp_err.h"
#include "bme280_parse.h"

/* BME280 temperature/pressure/humidity sensor over the shared i2c_bus (port 0).
 * The bus must be brought up first with i2c_bus_init(). SDK glue — compile-gated
 * only; the pure compensation lives in bme280_parse.{h,c}. */

/* The two possible 7-bit I2C addresses (SDO -> GND / VDDIO). */
#define BME280_ADDR_PRIMARY   0x76
#define BME280_ADDR_SECONDARY 0x77

typedef struct {
    uint8_t      addr;   /* 7-bit I2C address */
    bme280_calib calib;  /* trimming coefficients, read at init */
} bme280;

/* Probe the chip id, soft-reset, read+parse the calib registers, and configure
 * forced-mode oversampling x1 on all channels. ESP_OK / ESP_ERR_NOT_FOUND (bad
 * chip id) / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t bme280_init(bme280 *self, uint8_t addr);

/* Trigger a forced measurement, read the raw frame, and return the compensated
 * SI reading. ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t bme280_read(bme280 *self, bme280_reading *out);

#endif /* CLIBS_BME280_H */
