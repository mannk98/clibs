#ifndef CLIBS_RC5_RX_H
#define CLIBS_RC5_RX_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "rc5.h"

/* RC5 IR receiver (active-low demodulator output, e.g. VS1838B) on one GPIO. The
 * caller owns this struct (no malloc); `halfbits` holds one frame's 28 Manchester
 * half-bit samples (each 0/1) for rc5_decode. Not reentrant. */
typedef struct {
    gpio_num_t pin;
    uint8_t    halfbits[28];
} rc5_rx;

/* Configure `pin` as an input with pull-up (demodulator idles high). NULL self
 * -> ESP_ERR_INVALID_ARG. */
esp_err_t rc5_rx_init(rc5_rx *self, gpio_num_t pin);

/* Sample the line at the RC5 half-bit period (~889us) into halfbits, then
 * rc5_decode into *out. Returns ESP_OK on a decoded frame, ESP_ERR_TIMEOUT if no
 * start edge arrived within timeout_ms, ESP_ERR_INVALID_CRC if a frame was
 * captured but failed to decode, or ESP_ERR_INVALID_ARG on NULL self/out. */
esp_err_t rc5_rx_read(rc5_rx *self, rc5_result *out, uint32_t timeout_ms);

#endif /* CLIBS_RC5_RX_H */
