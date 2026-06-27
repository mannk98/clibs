#include "ir_nec_rx.h"
#include "esp_timer.h"   /* esp_timer_get_time (microseconds) */

/* Poll the demodulator pin (idle high, active low) and record the duration
 * between edges into buf, until an inter-edge gap exceeds the NEC frame end
 * (~20ms) or the timeout elapses. Polling capture is a FIRST CUT — on hardware
 * prefer a GPIO edge ISR timestamping into buf with interrupts handled. This
 * file is SDK glue: it is COMPILE_GATE only (not host-runnable) and is verified
 * solely by the deferred esp-gate link. */

esp_err_t ir_nec_rx_init(ir_nec_rx *self, gpio_num_t pin)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);   /* idle high */
    return ESP_OK;
}

esp_err_t ir_nec_rx_read(ir_nec_rx *self, ir_nec_result *out, uint32_t timeout_ms)
{
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;

    /* Wait for the falling edge that starts the leading mark. */
    while (gpio_get_level(self->pin) == 1) {
        if (esp_timer_get_time() > deadline) {
            return ESP_ERR_TIMEOUT;
        }
    }

    size_t  n = 0;
    int     last = 0;                      /* current level (just went low) */
    int64_t t_prev = esp_timer_get_time();

    while (n < (sizeof self->buf / sizeof self->buf[0])) {
        int     level = gpio_get_level(self->pin);
        int64_t now = esp_timer_get_time();
        if (level != last) {
            int64_t dur = now - t_prev;
            self->buf[n++] = (uint16_t)(dur > 65535 ? 65535 : dur);
            t_prev = now;
            last = level;
        } else if (now - t_prev > 20000) { /* end of frame (long idle) */
            break;
        }
        if (now > deadline) {
            break;
        }
    }

    if (n < 66) {
        return ESP_ERR_TIMEOUT;
    }
    return ir_nec_decode(self->buf, n, out) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
