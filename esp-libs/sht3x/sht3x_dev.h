#ifndef CLIBS_SHT3X_DEV_H
#define CLIBS_SHT3X_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "sht3x.h"

/* SHT3x temperature/humidity sensor over the shared i2c_bus (port 0). The bus
 * must be brought up first with i2c_bus_init(). SDK glue — compile-gated only;
 * the pure raw->engineering-units conversion lives in sht3x.{h,c}
 * (sht3x_parse). */

/* The two possible 7-bit I2C addresses (ADDR pin -> GND / VDD). */
#define SHT3X_ADDR_LOW  0x44
#define SHT3X_ADDR_HIGH 0x45

typedef struct {
    uint8_t addr;   /* 7-bit I2C address */
} sht3x;

/* Bind `self` to the device at `addr`. No bus traffic — measurement commands
 * are issued per-read. ESP_OK / ESP_ERR_INVALID_ARG (NULL). */
esp_err_t sht3x_init(sht3x *self, uint8_t addr);

/* Trigger a single-shot high-repeatability measurement, wait for it, read the
 * 6-byte frame and convert via sht3x_parse.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / ESP_FAIL (CRC) / bus error.
 * NOTE (hardware-deferred): the ~15 ms measurement delay between the
 * write-command and the read is a first cut to tune on real hardware. */
esp_err_t sht3x_read(sht3x *self, int32_t *temp_mc, int32_t *humi_mrh);

#endif /* CLIBS_SHT3X_DEV_H */
