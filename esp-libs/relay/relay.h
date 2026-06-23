#ifndef CLIBS_RELAY_H
#define CLIBS_RELAY_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "relay_level.h"

typedef struct {
    gpio_num_t pin;
    bool       active_low;
    bool       on;
} relay;

/* Configure `pin` as a push-pull output and drive the relay OFF. */
esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low);
esp_err_t relay_set(relay *self, bool on);
esp_err_t relay_toggle(relay *self);
bool      relay_is_on(const relay *self);

#endif /* CLIBS_RELAY_H */
