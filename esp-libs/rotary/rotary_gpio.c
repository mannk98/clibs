#include "rotary_gpio.h"

/* Sample A and B into a 2-bit code (a<<1)|b. Levels are normalised to 0/1 first so
 * the shift operates on a non-negative value. */
static uint8_t rotary_gpio_read(gpio_num_t a, gpio_num_t b) {
    uint8_t la = (gpio_get_level(a) != 0) ? 1u : 0u;
    uint8_t lb = (gpio_get_level(b) != 0) ? 1u : 0u;
    return (uint8_t)((la << 1) | lb);
}

esp_err_t rotary_gpio_init(rotary_gpio *self, gpio_num_t pin_a, gpio_num_t pin_b) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin_a = pin_a;
    self->pin_b = pin_b;
    gpio_set_direction(pin_a, GPIO_MODE_INPUT);
    gpio_set_direction(pin_b, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin_a, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pin_b, GPIO_PULLUP_ONLY);
    rotary_init(&self->enc, rotary_gpio_read(pin_a, pin_b));
    return ESP_OK;
}

rotary_dir rotary_gpio_poll(rotary_gpio *self) {
    if (!self) {
        return ROTARY_NONE;
    }
    return rotary_update(&self->enc, rotary_gpio_read(self->pin_a, self->pin_b));
}
