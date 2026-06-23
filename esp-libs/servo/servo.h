#ifndef CLIBS_SERVO_H
#define CLIBS_SERVO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "servo_duty.h"

/* Software-PWM servo at 50 Hz (period 20000 us). NOTE: the SDK PWM is one global
 * subsystem (channel 0) — servo and pwm_dimmer cannot both run. */
typedef struct { uint8_t angle; bool started; } servo;

esp_err_t servo_init(servo *self, gpio_num_t pin);
esp_err_t servo_set(servo *self, uint8_t angle);

#endif /* CLIBS_SERVO_H */
