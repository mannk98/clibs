#include "ds18b20.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* 1-Wire on a single open-drain pin: drive low via OUTPUT, release via INPUT (pull-up). */
static void ow_low(gpio_num_t pin)     { gpio_set_direction(pin, GPIO_MODE_OUTPUT); gpio_set_level(pin, 0); }
static void ow_release(gpio_num_t pin) { gpio_set_direction(pin, GPIO_MODE_INPUT); }

static bool ow_reset(gpio_num_t pin)
{
    ow_low(pin);
    ets_delay_us(480);
    ow_release(pin);
    ets_delay_us(70);
    bool present = (gpio_get_level(pin) == 0);
    ets_delay_us(410);
    return present;
}

static void ow_write_bit(gpio_num_t pin, int bit)
{
    ow_low(pin);
    if (bit) {
        ets_delay_us(6);
        ow_release(pin);
        ets_delay_us(64);
    } else {
        ets_delay_us(60);
        ow_release(pin);
        ets_delay_us(10);
    }
}

static int ow_read_bit(gpio_num_t pin)
{
    ow_low(pin);
    ets_delay_us(6);
    ow_release(pin);
    ets_delay_us(9);
    int b = gpio_get_level(pin);
    ets_delay_us(55);
    return b;
}

static void ow_write_byte(gpio_num_t pin, uint8_t v)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(pin, v & 1);
        v >>= 1;
    }
}

static uint8_t ow_read_byte(gpio_num_t pin)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) {
        v >>= 1;
        if (ow_read_bit(pin)) {
            v |= 0x80;
        }
    }
    return v;
}

esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    ow_release(pin);
    return ESP_OK;
}

esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc)
{
    if (self == NULL || temp_mc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t pin = self->pin;
    if (!ow_reset(pin)) {
        return ESP_ERR_TIMEOUT;
    }
    ow_write_byte(pin, 0xCC);   /* skip ROM */
    ow_write_byte(pin, 0x44);   /* convert T */
    ets_delay_us(750000);       /* up to 750 ms for 12-bit; tune on hardware (prefer vTaskDelay) */
    if (!ow_reset(pin)) {
        return ESP_ERR_TIMEOUT;
    }
    ow_write_byte(pin, 0xCC);   /* skip ROM */
    ow_write_byte(pin, 0xBE);   /* read scratchpad */
    uint8_t scratch[9];
    for (int i = 0; i < 9; i++) {
        scratch[i] = ow_read_byte(pin);
    }
    if (!ds18b20_parse_temp(scratch, temp_mc)) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
