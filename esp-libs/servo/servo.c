#include "servo.h"

#include "driver/pwm.h"

#define SERVO_PERIOD_US 20000

esp_err_t servo_init(servo *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->angle = 90;
    self->started = false;
    uint32_t pins[1]   = { (uint32_t) pin };
    uint32_t duties[1] = { servo_angle_to_duty(90) };
    esp_err_t err = pwm_init(SERVO_PERIOD_US, duties, 1, pins);
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

esp_err_t servo_set(servo *self, uint8_t angle)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = pwm_set_duty(0, servo_angle_to_duty(angle));
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();
    if (err == ESP_OK) {
        self->angle = angle;
    }
    return err;
}
