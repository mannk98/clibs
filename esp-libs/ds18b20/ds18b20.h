#ifndef CLIBS_DS18B20_H
#define CLIBS_DS18B20_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ds18b20_parse.h"

typedef struct { gpio_num_t pin; } ds18b20;

esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin);
/* Single-drop 1-Wire read; fills milli-deg C. ESP_OK / ESP_ERR_TIMEOUT (no presence) /
 * ESP_ERR_INVALID_CRC (bad scratchpad). */
esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc);

#endif /* CLIBS_DS18B20_H */
