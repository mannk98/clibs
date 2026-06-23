#include "relay.h"

esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    self->active_low = active_low;
    self->on = false;
    esp_err_t err = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) {
        return err;
    }
    return relay_set(self, false);
}

esp_err_t relay_set(relay *self, bool on)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = gpio_set_level(self->pin, (uint32_t) relay_level(on, self->active_low));
    if (err == ESP_OK) {
        self->on = on;
    }
    return err;
}

esp_err_t relay_toggle(relay *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return relay_set(self, !self->on);
}

bool relay_is_on(const relay *self)
{
    return (self != NULL) && self->on;
}
