#ifndef CLIBS_PIR_GPIO_H
#define CLIBS_PIR_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "pir.h"

/* PIR sensor on one GPIO + the software hold FSM. Not reentrant. */
typedef struct { pir fsm; gpio_num_t pin; bool active_high; } pir_gpio;

/* Configure the pin as input and init the FSM with hold_ms. Returns
 * ESP_ERR_INVALID_ARG if self is NULL, else ESP_OK. */
esp_err_t pir_gpio_init(pir_gpio *self, gpio_num_t pin, uint32_t hold_ms, bool active_high);

/* Read the pin and run the FSM with the caller's monotonic now_ms.
 * Returns the pir_event (PIR_NONE if self is NULL). */
pir_event pir_gpio_poll(pir_gpio *self, uint32_t now_ms);

#endif /* CLIBS_PIR_GPIO_H */
