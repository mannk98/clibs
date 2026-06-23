#ifndef CLIBS_BUTTON_H
#define CLIBS_BUTTON_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "debounce.h"   /* L1 debounce FSM */

typedef enum { BUTTON_NONE, BUTTON_PRESSED, BUTTON_RELEASED } button_event;

typedef struct {
    gpio_num_t pin;
    bool       active_low;
    debounce   db;
} button;

/* Configure `pin` as input (pull-up if active_low) and init the debounce FSM. */
esp_err_t button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold);

/* Sample the pin once (call at a fixed interval). Returns a debounced edge. */
button_event button_poll(button *self);

/* Current debounced pressed state. */
bool button_is_pressed(const button *self);

#endif /* CLIBS_BUTTON_H */
