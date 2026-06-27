#include "rc5_rx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"   /* esp_timer_get_time (microseconds) */

/* RC5 transmits 14 bits as 28 Manchester half-bits at a 1/2 bit-time of ~889us
 * (one full bit = 1.778ms). The line idles high; the leading start bit S1 = 1 is
 * the pair (0,1), so the frame opens with a falling edge. After detecting that
 * edge we re-align to the centre of the first half-bit and sample the (inverted,
 * active-low) line once per ~889us into halfbits, then hand it to rc5_decode.
 * Polling capture is a FIRST CUT — on hardware prefer a GPIO edge ISR. This file
 * is SDK glue: it is COMPILE_GATE only (not host-runnable) and is verified solely
 * by the deferred esp-gate link. */

#define RC5_HALF_US   889u   /* RC5 half-bit period (microseconds) */
#define RC5_HALFBITS  28u

esp_err_t rc5_rx_init(rc5_rx *self, gpio_num_t pin)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);   /* idle high */
    return ESP_OK;
}

esp_err_t rc5_rx_read(rc5_rx *self, rc5_result *out, uint32_t timeout_ms)
{
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;

    /* Wait for the falling edge that starts the leading half-bit. */
    while (gpio_get_level(self->pin) == 1) {
        if (esp_timer_get_time() > deadline) {
            return ESP_ERR_TIMEOUT;
        }
    }

    /* Re-align to the centre of the first half-bit, then sample every ~889us.
     * The demodulator output is active-low, so a low line is a logical 1
     * half-bit; invert the level into the 0/1 convention rc5_decode expects. */
    int64_t t_sample = esp_timer_get_time() + RC5_HALF_US / 2;
    for (size_t i = 0; i < RC5_HALFBITS; i++) {
        while (esp_timer_get_time() < t_sample) {
            /* busy-wait to the sample instant */
        }
        self->halfbits[i] = (uint8_t)(gpio_get_level(self->pin) == 0 ? 1u : 0u);
        t_sample += RC5_HALF_US;
    }

    return rc5_decode(self->halfbits, RC5_HALFBITS, out) ? ESP_OK
                                                         : ESP_ERR_INVALID_CRC;
}
