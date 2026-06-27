#include "hx711_gpio.h"

#include "rom/ets_sys.h"   /* ets_delay_us */

esp_err_t hx711_dev_init(hx711_dev *self, gpio_num_t sck, gpio_num_t dout, hx711_gain gain)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->sck  = sck;
    self->dout = dout;
    self->gain = gain;

    gpio_set_direction(sck, GPIO_MODE_OUTPUT);
    gpio_set_direction(dout, GPIO_MODE_INPUT);
    gpio_set_level(sck, 0);   /* idle low; a >60us high puts HX711 to sleep */
    return ESP_OK;
}

esp_err_t hx711_dev_read(hx711_dev *self, int32_t *out, uint32_t timeout_ms)
{
    if (self == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* DOUT goes low when a conversion is ready. Poll at ~1ms granularity.
     * NOTE: this timing is a first cut — validate on real hardware. */
    uint32_t waited = 0;
    while (gpio_get_level(self->dout) != 0) {
        if (waited >= timeout_ms) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1000);
        waited++;
    }

    /* Clock out 24 data bits, MSB first. Each SCK rising edge shifts the next
     * bit onto DOUT; read it while SCK is high. */
    uint8_t raw[3] = {0};
    for (int i = 0; i < 24; i++) {
        gpio_set_level(self->sck, 1);
        ets_delay_us(1);
        int bit = gpio_get_level(self->dout);
        gpio_set_level(self->sck, 0);
        ets_delay_us(1);

        int idx = i / 8;
        raw[idx] = (uint8_t)(raw[idx] << 1);
        if (bit) {
            raw[idx] |= 1u;
        }
    }

    /* Extra pulses select channel/gain for the next conversion. */
    for (int i = 0; i < (int)self->gain; i++) {
        gpio_set_level(self->sck, 1);
        ets_delay_us(1);
        gpio_set_level(self->sck, 0);
        ets_delay_us(1);
    }

    *out = hx711_parse(raw);
    return ESP_OK;
}
