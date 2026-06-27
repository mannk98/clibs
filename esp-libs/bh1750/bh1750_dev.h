#ifndef CLIBS_BH1750_DEV_H
#define CLIBS_BH1750_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "bh1750.h"

/* BH1750 ambient-light sensor over the shared i2c_bus (port 0). The bus must be
 * brought up first with i2c_bus_init(). SDK glue — compile-gated only; the pure
 * raw->milli_lux conversion lives in bh1750.{h,c} (bh1750_parse). */

/* The two possible 7-bit I2C addresses (ADDR pin -> GND / VDD). */
#define BH1750_ADDR_LOW  0x23
#define BH1750_ADDR_HIGH 0x5C

typedef struct {
    uint8_t addr;   /* 7-bit I2C address */
} bh1750;

/* Set continuous high-resolution mode (opcode 0x10) on the device at `addr`.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / bus error.
 * NOTE (hardware-deferred): first H-res conversion takes ~120 ms; allow that
 * settling time before the first bh1750_read(). */
esp_err_t bh1750_init(bh1750 *self, uint8_t addr);

/* Read 2 bytes from the device and convert to milli-lux via bh1750_parse.
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / ESP_FAIL (parse) / bus error. */
esp_err_t bh1750_read(bh1750 *self, uint32_t *milli_lux);

#endif /* CLIBS_BH1750_DEV_H */
