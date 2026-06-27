#ifndef CLIBS_SK6812_DEV_H
#define CLIBS_SK6812_DEV_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "sk6812.h"

/* SK6812 RGBW strip on one GPIO. grbw_buf is caller storage >= 4*count bytes
 * (no malloc). Not reentrant. */
typedef struct { gpio_num_t pin; uint16_t count; uint8_t *grbw; size_t grbw_size; } sk6812;

esp_err_t sk6812_init(sk6812 *self, gpio_num_t pin, uint16_t count, uint8_t *grbw_buf, size_t grbw_size);
/* Render px (count pixels) at brightness into grbw, then bit-bang it out the pin. */
esp_err_t sk6812_show(sk6812 *self, const sk6812_rgbw *px, uint8_t brightness);

#endif /* CLIBS_SK6812_DEV_H */
