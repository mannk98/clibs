#ifndef CLIBS_DS3231_DEV_H
#define CLIBS_DS3231_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "ds3231.h"

/* DS3231 real-time clock over the shared i2c_bus (port 0). The bus must be
 * brought up first with i2c_bus_init(). SDK glue — compile-gated only; the pure
 * BCD decode/encode + temperature conversion lives in ds3231.{h,c}. */

/* Fixed 7-bit I2C address of the DS3231. */
#define DS3231_ADDR 0x68

typedef struct {
    uint8_t addr;   /* 7-bit I2C address */
} ds3231;

/* Bind the driver to the device at `addr` (use DS3231_ADDR == 0x68).
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL). */
esp_err_t ds3231_init(ds3231 *self, uint8_t addr);

/* Read the 7 timekeeping registers (0x00..0x06) and decode into *out.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / ESP_FAIL (decode) / bus error. */
esp_err_t ds3231_get(ds3231 *self, ds3231_time *out);

/* Encode *t and write the 7 timekeeping registers (0x00..0x06).
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / ESP_FAIL (encode) / bus error. */
esp_err_t ds3231_set(ds3231 *self, const ds3231_time *t);

/* Read the 2 on-chip temperature registers (0x11..0x12) and convert to
 * milli-degrees Celsius via ds3231_temp_mc.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error. */
esp_err_t ds3231_read_temp(ds3231 *self, int32_t *milli_c);

#endif /* CLIBS_DS3231_DEV_H */
