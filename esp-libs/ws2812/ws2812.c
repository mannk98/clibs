#include "ws2812.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* WS2812 @800kHz: T0H~0.35us/T0L~0.8us, T1H~0.7us/T1L~0.6us, reset >50us. ets_delay_us
 * is too coarse for the sub-us highs — this is a FIRST CUT structure; on hardware
 * replace the per-bit highs/lows with a cycle-counted (xthal_get_ccount) inline routine
 * with interrupts disabled. Verified only by the deferred esp-gate link. */
esp_err_t ws2812_init(ws2812 *self, gpio_num_t pin, uint16_t count, uint8_t *grb_buf, size_t grb_size) {
    if (!self || !grb_buf) {
        return ESP_ERR_INVALID_ARG;
    }
    if (grb_size < (size_t)count * 3u) {
        return ESP_ERR_INVALID_SIZE;
    }
    self->pin = pin;
    self->count = count;
    self->grb = grb_buf;
    self->grb_size = grb_size;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    return ESP_OK;
}

static void ws2812_send_byte(gpio_num_t pin, uint8_t b) {
    for (int i = 7; i >= 0; i--) {        /* MSB first */
        if (b & (1u << i)) {
            gpio_set_level(pin, 1); ets_delay_us(1); gpio_set_level(pin, 0);
        } else {
            gpio_set_level(pin, 1); gpio_set_level(pin, 0); ets_delay_us(1);
        }
    }
}

esp_err_t ws2812_show(ws2812 *self, const ws2812_rgb *px, uint8_t brightness) {
    if (!self || !px) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t n = ws2812_render(px, self->count, brightness, self->grb, self->grb_size);
    if (n == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    for (size_t i = 0; i < n; i++) {
        ws2812_send_byte(self->pin, self->grb[i]);
    }
    ets_delay_us(60);                     /* latch/reset */
    return ESP_OK;
}
