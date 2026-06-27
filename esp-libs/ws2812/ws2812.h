#ifndef CLIBS_WS2812_H
#define CLIBS_WS2812_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ws2812_render.h"

/* WS2812 strip on one GPIO. grb_buf is caller storage >= 3*count bytes (no malloc). Not reentrant. */
typedef struct { gpio_num_t pin; uint16_t count; uint8_t *grb; size_t grb_size; } ws2812;

esp_err_t ws2812_init(ws2812 *self, gpio_num_t pin, uint16_t count, uint8_t *grb_buf, size_t grb_size);
/* Render px (count pixels) at brightness into grb, then bit-bang it out the pin. */
esp_err_t ws2812_show(ws2812 *self, const ws2812_rgb *px, uint8_t brightness);

#endif /* CLIBS_WS2812_H */
