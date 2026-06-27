#ifndef CLIBS_ROTARY_GPIO_H
#define CLIBS_ROTARY_GPIO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "rotary.h"

/* Quadrature encoder on two GPIO (A/B). Caller-owned storage. Not reentrant. */
typedef struct { rotary enc; gpio_num_t pin_a; gpio_num_t pin_b; } rotary_gpio;

/* Configure both pins as pulled-up inputs and seed the decoder from their current
 * level. NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t  rotary_gpio_init(rotary_gpio *self, gpio_num_t pin_a, gpio_num_t pin_b);
/* Read A/B -> (a<<1)|b -> rotary_update. NULL self -> ROTARY_NONE. */
rotary_dir rotary_gpio_poll(rotary_gpio *self);

#endif /* CLIBS_ROTARY_GPIO_H */
