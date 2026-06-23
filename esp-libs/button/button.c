#include "button.h"

esp_err_t button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    self->active_low = active_low;
    esp_err_t err = gpio_set_direction(pin, GPIO_MODE_INPUT);
    if (err != ESP_OK) {
        return err;
    }
    /* ESP8266 has an internal pull-up only (no pull-down on most pins); for an
       active-high button add an external pull-down resistor. */
    err = gpio_set_pull_mode(pin, active_low ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
    if (err != ESP_OK) {
        return err;
    }
    debounce_init(&self->db, threshold ? threshold : 1, 0);
    return ESP_OK;
}

button_event button_poll(button *self)
{
    if (self == NULL) {
        return BUTTON_NONE;
    }
    int level = gpio_get_level(self->pin);
    uint8_t pressed_raw = self->active_low ? (level == 0) : (level != 0);
    debounce_edge edge = debounce_update(&self->db, pressed_raw);
    if (edge == DEBOUNCE_RISING) {
        return BUTTON_PRESSED;
    }
    if (edge == DEBOUNCE_FALLING) {
        return BUTTON_RELEASED;
    }
    return BUTTON_NONE;
}

bool button_is_pressed(const button *self)
{
    if (self == NULL) {
        return false;
    }
    return debounce_level(&self->db) != 0;
}
