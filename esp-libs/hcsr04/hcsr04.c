#include "hcsr04.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

#define HCSR04_TIMEOUT_US 40000   /* ~6.8 m round trip */

esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->trig = trig;
    self->echo = echo;
    gpio_set_direction(trig, GPIO_MODE_OUTPUT);
    gpio_set_level(trig, 0);
    gpio_set_direction(echo, GPIO_MODE_INPUT);
    return ESP_OK;
}

esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm)
{
    if (self == NULL || mm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t trig = self->trig;
    gpio_num_t echo = self->echo;

    /* 10 us trigger pulse */
    gpio_set_level(trig, 1);
    ets_delay_us(10);
    gpio_set_level(trig, 0);

    /* wait for the echo rising edge */
    uint32_t waited = 0;
    while (gpio_get_level(echo) == 0) {
        if (waited >= HCSR04_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
        waited++;
    }
    /* measure the echo-high duration */
    uint32_t dur = 0;
    while (gpio_get_level(echo) != 0) {
        if (dur >= HCSR04_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
        dur++;
    }
    *mm = hcsr04_us_to_mm(dur);
    return ESP_OK;
}
