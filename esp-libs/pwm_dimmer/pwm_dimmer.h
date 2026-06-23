#ifndef CLIBS_PWM_DIMMER_H
#define CLIBS_PWM_DIMMER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "dimmer_duty.h"

/* Single-dimmer wrapper over the SDK software PWM. NOTE: the SDK PWM is a single
 * global subsystem; this wrapper assumes ONE dimmer on channel 0. */
typedef struct {
    uint32_t period;
    uint32_t max_duty;
    uint8_t  percent;
    bool     started;
} pwm_dimmer;

esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us);
esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent);

#endif /* CLIBS_PWM_DIMMER_H */
