#ifndef CLIBS_MAX7219_H
#define CLIBS_MAX7219_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "spi_bus.h"
#include "max7219_encode.h"

/* MAX7219 over spi_bus, selected by its own software CS GPIO -> multiple MAX7219
 * (and other SPI) devices coexist on the shared bus. Caller-owned storage, no
 * malloc. Not reentrant. */
typedef struct { spi_device dev; uint8_t intensity; } max7219;

esp_err_t max7219_init(max7219 *self, gpio_num_t cs_pin, uint8_t intensity);   /* intensity 0..15 */
esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp);  /* pos 0..7 */
esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits);  /* 8x8 matrix row 0..7 */

#endif /* CLIBS_MAX7219_H */
