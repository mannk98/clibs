#ifndef CLIBS_SPI_BUS_H
#define CLIBS_SPI_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"

/* Initialise HSPI as master (mode 0, 8 MHz). The hardware chip-select is
 * DISABLED: ESP8266 HSPI has a single hardware CS, so multiple SPI peripherals
 * share the bus via a per-device software CS (see spi_device). */
esp_err_t spi_bus_init(void);

/* Raw full-duplex transfer of `len` bytes (1..64) over the shared bus with NO
 * chip-select asserted — the caller (or spi_device_transfer) owns CS. rx may be
 * NULL for write-only. len 0 or > 64 -> ESP_ERR_INVALID_ARG (64 B is the SDK
 * per-transaction limit). */
esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

/* A SPI peripheral on the shared bus, selected by its own software CS GPIO
 * (active-low). Caller-owned storage, no malloc. Not reentrant. */
typedef struct { gpio_num_t cs_pin; } spi_device;

/* Configure the CS GPIO as an output and drive it HIGH (idle / deasserted). */
esp_err_t spi_device_init(spi_device *self, gpio_num_t cs_pin);

/* Assert CS (drive low) -> spi_bus_transfer(tx, rx, len) -> deassert CS (drive
 * high, always, even if the transfer failed). NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t spi_device_transfer(spi_device *self, const uint8_t *tx, uint8_t *rx, size_t len);

#endif /* CLIBS_SPI_BUS_H */
