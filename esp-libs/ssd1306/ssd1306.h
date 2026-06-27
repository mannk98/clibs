#ifndef CLIBS_SSD1306_H
#define CLIBS_SSD1306_H

#include <stdint.h>
#include "esp_err.h"
#include "ssd1306_fb.h"

/* Default SSD1306 7-bit I2C address (SA0 low). */
#define SSD1306_ADDR 0x3C

/* SSD1306 OLED over i2c_bus. buf is caller storage >= SSD1306_FB_BYTES(width,height).
 * Not reentrant. */
typedef struct {
    uint8_t addr;
    ssd1306_fb fb;
} ssd1306;

/* Bind self, init the framebuffer over caller-owned buf, and run the SSD1306
 * power-on command sequence over i2c_bus. NULL self/buf -> ESP_ERR_INVALID_ARG. */
esp_err_t ssd1306_init(ssd1306 *self, uint8_t addr, uint8_t *buf, uint8_t width, uint8_t height);

/* Set the addressing window and stream the framebuffer to the panel over i2c_bus.
 * NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t ssd1306_show(ssd1306 *self);

#endif /* CLIBS_SSD1306_H */
