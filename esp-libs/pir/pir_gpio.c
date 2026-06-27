#include "pir_gpio.h"

esp_err_t pir_gpio_init(pir_gpio *self, gpio_num_t pin, uint32_t hold_ms, bool active_high) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    pir_init(&self->fsm, hold_ms);
    self->pin = pin;
    self->active_high = active_high;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    /* ESP8266 has no internal pull-down: an active-high PIR (push-pull output) needs
     * no pull; an active-low wiring uses the internal pull-up. */
    gpio_set_pull_mode(pin, active_high ? GPIO_FLOATING : GPIO_PULLUP_ONLY);
    return ESP_OK;
}

pir_event pir_gpio_poll(pir_gpio *self, uint32_t now_ms) {
    if (!self) {
        return PIR_NONE;
    }
    int level = gpio_get_level(self->pin);
    bool motion = self->active_high ? (level != 0) : (level == 0);
    return pir_update(&self->fsm, motion, now_ms);
}
