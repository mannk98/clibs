#ifndef CLIBS_IR_NEC_RX_H
#define CLIBS_IR_NEC_RX_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ir_nec.h"

/* IR receiver (active-low demodulator output, e.g. VS1838B) on one GPIO. The
 * caller owns this struct (no malloc); `buf` holds the captured mark/space
 * durations for one frame (66 needed; 70 slack). Not reentrant. */
typedef struct {
    gpio_num_t pin;
    uint16_t   buf[70];
} ir_nec_rx;

/* Configure `pin` as an input with pull-up (demodulator idles high). NULL self
 * -> ESP_ERR_INVALID_ARG. */
esp_err_t ir_nec_rx_init(ir_nec_rx *self, gpio_num_t pin);

/* Capture one frame's mark/space durations (bounded by timeout_ms) into buf,
 * then ir_nec_decode into *out. Returns ESP_OK on a decoded frame,
 * ESP_ERR_TIMEOUT if no full frame arrived, ESP_ERR_INVALID_CRC if a frame was
 * captured but failed to decode, or ESP_ERR_INVALID_ARG on NULL self/out. */
esp_err_t ir_nec_rx_read(ir_nec_rx *self, ir_nec_result *out, uint32_t timeout_ms);

#endif /* CLIBS_IR_NEC_RX_H */
