#ifndef CLIBS_TM1637_DEV_H
#define CLIBS_TM1637_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "tm1637.h"

/* SDK glue for a 4-digit TM1637 7-segment display over its 2-wire (CLK/DIO)
 * protocol. COMPILE_GATE only (xtensa) — NOT host-compiled. The pure segment
 * encode is in tm1637.{h,c} (tm1637_digit_segments / tm1637_encode_number).
 * Caller-owned storage, no malloc. Not reentrant. */
typedef struct {
    gpio_num_t clk;
    gpio_num_t dio;
    uint8_t    brightness;   /* 0..7 */
} tm1637;

/* Configure CLK/DIO and store brightness (clamped to 0..7).
 * NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t tm1637_init(tm1637 *self, gpio_num_t clk, gpio_num_t dio, uint8_t brightness);

/* Push 4 segment bytes (segments[0]=leftmost .. segments[3]=rightmost) to the
 * display: data command 0x40 (auto-increment), address 0xC0 + four data bytes,
 * then display-control 0x88 | brightness. Each transfer is framed by start /
 * stop and each byte is followed by an ACK clock.
 * NULL self/segments -> ESP_ERR_INVALID_ARG. */
esp_err_t tm1637_show(tm1637 *self, const uint8_t segments[4]);

#endif /* CLIBS_TM1637_DEV_H */
