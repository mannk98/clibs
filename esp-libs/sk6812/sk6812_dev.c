#include "sk6812_dev.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* SK6812 RGBW @800kHz, 32 bits/pixel (GRBW): T0H~0.3us/T0L~0.9us,
 * T1H~0.6us/T1L~0.6us, reset/latch >=80us. ets_delay_us is too coarse for the
 * sub-us highs — this is a FIRST CUT structure mirroring ws2812; on hardware
 * replace the per-bit highs/lows with a cycle-counted (xthal_get_ccount) inline
 * routine with interrupts disabled. Verified only by the deferred esp-gate link. */
esp_err_t sk6812_init(sk6812 *self, gpio_num_t pin, uint16_t count, uint8_t *grbw_buf, size_t grbw_size) {
    if (!self || !grbw_buf) {
        return ESP_ERR_INVALID_ARG;
    }
    if (grbw_size < (size_t)count * 4u) {
        return ESP_ERR_INVALID_SIZE;
    }
    self->pin = pin;
    self->count = count;
    self->grbw = grbw_buf;
    self->grbw_size = grbw_size;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    return ESP_OK;
}

static void sk6812_send_byte(gpio_num_t pin, uint8_t b) {
    for (int i = 7; i >= 0; i--) {        /* MSB first */
        if (b & (1u << i)) {
            gpio_set_level(pin, 1); ets_delay_us(1); gpio_set_level(pin, 0);
        } else {
            gpio_set_level(pin, 1); gpio_set_level(pin, 0); ets_delay_us(1);
        }
    }
}

esp_err_t sk6812_show(sk6812 *self, const sk6812_rgbw *px, uint8_t brightness) {
    if (!self || !px) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t n = sk6812_render(px, self->count, brightness, self->grbw, self->grbw_size);
    if (n == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    for (size_t i = 0; i < n; i++) {       /* 4 bytes = 32 bits per pixel */
        sk6812_send_byte(self->pin, self->grbw[i]);
    }
    ets_delay_us(80);                      /* latch/reset >= 80us */
    return ESP_OK;
}
