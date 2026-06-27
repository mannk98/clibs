#ifndef CLIBS_HX711_GPIO_H
#define CLIBS_HX711_GPIO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "hx711.h"

/* SDK glue for the HX711: SCK/DOUT bit-bang to read one 24-bit sample.
 * COMPILE_GATE only (xtensa) — not host-compiled. Pure conversion is in
 * hx711.{h,c} (hx711_parse). */

/* Channel/gain select, encoded as the number of extra SCK pulses sent after
 * the 24 data clocks (the value also picks the next conversion's input). */
typedef enum {
    HX711_GAIN_A128 = 1,   /* channel A, gain 128 */
    HX711_GAIN_B32  = 2,   /* channel B, gain 32  */
    HX711_GAIN_A64  = 3    /* channel A, gain 64  */
} hx711_gain;

typedef struct {
    gpio_num_t sck;
    gpio_num_t dout;
    hx711_gain gain;
} hx711_dev;

/* Configure the SCK (output) and DOUT (input) pins and store the gain.
 * NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t hx711_dev_init(hx711_dev *self, gpio_num_t sck, gpio_num_t dout, hx711_gain gain);

/* Wait for DOUT-ready (timeout_ms budget), bit-bang 24 data bits plus the
 * gain pulses, and convert via hx711_parse into *out.
 * NULL self/out -> ESP_ERR_INVALID_ARG; no ready within budget -> ESP_ERR_TIMEOUT. */
esp_err_t hx711_dev_read(hx711_dev *self, int32_t *out, uint32_t timeout_ms);

#endif /* CLIBS_HX711_GPIO_H */
