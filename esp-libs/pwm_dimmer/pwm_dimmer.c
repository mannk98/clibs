#include "pwm_dimmer.h"

#include "driver/pwm.h"

esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->period = period_us;
    self->max_duty = period_us;   /* duty is in the same units as the period */
    self->percent = 0;
    self->started = false;

    uint32_t pins[1]   = { (uint32_t) pin };
    uint32_t duties[1] = { 0 };
    esp_err_t err = pwm_init(period_us, duties, 1, pins);
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();
    if (err != ESP_OK) {
        return err;
    }
    self->started = true;
    return ESP_OK;
}

esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = pwm_set_duty(0, dimmer_duty(percent, self->max_duty));
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();   /* re-apply the new duty */
    if (err == ESP_OK) {
        self->percent = percent;
    }
    return err;
}
