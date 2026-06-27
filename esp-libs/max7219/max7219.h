#ifndef CLIBS_MAX7219_H
#define CLIBS_MAX7219_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "max7219_encode.h"

/* MAX7219 over spi_bus (singleton bus -> no per-instance pin). Not reentrant. */
typedef struct { uint8_t intensity; } max7219;

esp_err_t max7219_init(max7219 *self, uint8_t intensity);             /* 0..15 */
esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp);  /* pos 0..7 */
esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits);  /* 8x8 matrix row 0..7 */

#endif /* CLIBS_MAX7219_H */
