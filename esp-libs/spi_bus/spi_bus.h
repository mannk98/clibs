#ifndef CLIBS_SPI_BUS_H
#define CLIBS_SPI_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* Initialise HSPI as master (mode 0, 8 MHz). */
esp_err_t spi_bus_init(void);
/* Full-duplex transfer of `len` bytes (1..64). rx may be NULL for write-only.
 * len 0 or > 64 -> ESP_ERR_INVALID_ARG (64 B is the SDK per-transaction limit). */
esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

#endif /* CLIBS_SPI_BUS_H */
