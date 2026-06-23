#ifndef CLIBS_HCSR04_H
#define CLIBS_HCSR04_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "hcsr04_dist.h"

typedef struct { gpio_num_t trig; gpio_num_t echo; } hcsr04;

esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo);
/* Trigger + measure echo; fills mm. ESP_OK / ESP_ERR_TIMEOUT (no echo within bound). */
esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm);

#endif /* CLIBS_HCSR04_H */
