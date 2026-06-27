#ifndef CLIBS_INA219_DEV_H
#define CLIBS_INA219_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "ina219.h"

/* INA219 current/power monitor over the shared i2c_bus (port 0). The bus must be
 * brought up first with i2c_bus_init(). SDK glue — compile-gated only; the pure
 * raw->unit conversions + calibration value live in ina219.{h,c}. */

/* Default 7-bit I2C address (A1=A0=GND). */
#define INA219_ADDR 0x40

typedef struct {
    uint8_t  addr;            /* 7-bit I2C address */
    uint32_t current_lsb_ua;  /* programmed current LSB, microamps */
} ina219;

/* Bind to the device at `addr`, derive the current LSB from the full-scale
 * current, write the config register (0x00 = 0x399F: 32 V range, /8 PGA,
 * 12-bit bus & shunt ADC, continuous) and the calibration register (0x05).
 *   current_lsb_ua = max_current_ma * 1000 / 32768
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ina219_init(ina219 *self, uint8_t addr,
                      uint32_t shunt_milliohm, uint32_t max_current_ma);

/* Read the bus-voltage register (0x02) and convert to millivolts.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ina219_read_bus_mv(ina219 *self, int32_t *bus_mv);

/* Read the shunt-voltage register (0x01) and convert to microvolts.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ina219_read_shunt_uv(ina219 *self, int32_t *shunt_uv);

/* Read the current register (0x04) and convert to microamps using the programmed
 * current LSB. ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ina219_read_current_ua(ina219 *self, int32_t *current_ua);

/* Read the power register (0x03) and convert to microwatts using the programmed
 * current LSB. ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ina219_read_power_uw(ina219 *self, int32_t *power_uw);

#endif /* CLIBS_INA219_DEV_H */
