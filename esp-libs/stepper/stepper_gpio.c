#include "stepper_gpio.h"

/* Write a half-step coil pattern (bits 0..3 = IN1..IN4) to the 4 coil GPIOs. */
static void stepper_gpio_write(const stepper_gpio *self, uint8_t coils)
{
    gpio_set_level(self->in1, (coils & 0x01u) ? 1 : 0);
    gpio_set_level(self->in2, (coils & 0x02u) ? 1 : 0);
    gpio_set_level(self->in3, (coils & 0x04u) ? 1 : 0);
    gpio_set_level(self->in4, (coils & 0x08u) ? 1 : 0);
}

esp_err_t stepper_gpio_init(stepper_gpio *self,
                            gpio_num_t in1, gpio_num_t in2,
                            gpio_num_t in3, gpio_num_t in4)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->in1 = in1;
    self->in2 = in2;
    self->in3 = in3;
    self->in4 = in4;

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << in1) | (1ULL << in2) |
                        (1ULL << in3) | (1ULL << in4),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) {
        return err;
    }

    stepper_init(&self->drv);
    stepper_gpio_write(self, stepper_coils(&self->drv));
    return ESP_OK;
}

esp_err_t stepper_gpio_step(stepper_gpio *self, bool forward)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t coils = stepper_step(&self->drv, forward);
    stepper_gpio_write(self, coils);
    return ESP_OK;
}
