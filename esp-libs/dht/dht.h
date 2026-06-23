#ifndef CLIBS_DHT_H
#define CLIBS_DHT_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "dht_parse.h"

typedef struct {
    gpio_num_t pin;
} dht;

esp_err_t dht_init(dht *self, gpio_num_t pin);

/* Bit-bang one read; on success fills deci-units. Returns ESP_OK,
 * ESP_ERR_TIMEOUT (no sensor response), or ESP_ERR_INVALID_CRC (bad checksum). */
esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct);

#endif /* CLIBS_DHT_H */
