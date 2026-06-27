#ifndef CLIBS_STEPPER_GPIO_H
#define CLIBS_STEPPER_GPIO_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "stepper.h"

/* ESP8266 SDK glue for the pure half-step sequencer. Drives the four coil
 * GPIOs (IN1..IN4) of a 4-wire unipolar 28BYJ-48 motor. Not reentrant.
 * COMPILE_GATE only — the pure logic lives in stepper.{h,c}. */
typedef struct {
    stepper    drv;             /* private: pure sequencer state */
    gpio_num_t in1, in2, in3, in4;
} stepper_gpio;

/* Configure the four coil GPIOs as outputs, reset the sequencer, and drive the
 * initial coil pattern (0x01). Returns ESP_ERR_INVALID_ARG if self is NULL. */
esp_err_t stepper_gpio_init(stepper_gpio *self,
                            gpio_num_t in1, gpio_num_t in2,
                            gpio_num_t in3, gpio_num_t in4);

/* Advance one half-step and write the new coil pattern to the 4 GPIOs.
 * Returns ESP_ERR_INVALID_ARG if self is NULL. */
esp_err_t stepper_gpio_step(stepper_gpio *self, bool forward);

#endif /* CLIBS_STEPPER_GPIO_H */
