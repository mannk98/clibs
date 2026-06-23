#include "dht.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* Spin until `pin` reaches `level`, up to `timeout_us`. Returns elapsed us, or -1. */
static int wait_level(gpio_num_t pin, int level, int timeout_us)
{
    int us = 0;
    while (gpio_get_level(pin) != level) {
        if (us >= timeout_us) {
            return -1;
        }
        ets_delay_us(1);
        us++;
    }
    return us;
}

esp_err_t dht_init(dht *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 1);   /* idle high */
    return ESP_OK;
}

esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct)
{
    if (self == NULL || temp_dc == NULL || humi_dpct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t pin = self->pin;
    uint8_t bytes[5] = {0};

    /* Start signal: hold low ~20 ms (covers DHT11/DHT22), release, then read. */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(20000);
    gpio_set_level(pin, 1);
    ets_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    /* Sensor response: ~80us low then ~80us high. */
    if (wait_level(pin, 0, 100) < 0) { return ESP_ERR_TIMEOUT; }
    if (wait_level(pin, 1, 100) < 0) { return ESP_ERR_TIMEOUT; }
    if (wait_level(pin, 0, 100) < 0) { return ESP_ERR_TIMEOUT; }

    /* 40 bits: each = ~50us low then a high pulse; long high (>~40us) = 1.
     * NOTE: this 1us-granularity timing is a first cut — tune on hardware. */
    for (int i = 0; i < 40; i++) {
        if (wait_level(pin, 1, 100) < 0) { return ESP_ERR_TIMEOUT; }
        int high_us = wait_level(pin, 0, 100);
        if (high_us < 0) { return ESP_ERR_TIMEOUT; }
        bytes[i / 8] = (uint8_t) (bytes[i / 8] << 1);
        if (high_us > 40) {
            bytes[i / 8] |= 1;
        }
    }

    if (!dht_parse(bytes, temp_dc, humi_dpct)) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
