#include "tm1637_dev.h"

#include "rom/ets_sys.h"   /* ets_delay_us */

/* Bit clock half-period. TM1637 tolerates a slow clock; this is a first-cut
 * delay to tune on real hardware. */
#define TM1637_BIT_DELAY_US 5u

/* DIO/CLK are driven open-drain style: to emit a 1 we release the line (let it
 * be pulled high) and to emit a 0 we drive it low. Pins are configured as
 * open-drain outputs so reads (for ACK) work too. */

static void tm1637_clk(tm1637 *self, int level)
{
    gpio_set_level(self->clk, level);
    ets_delay_us(TM1637_BIT_DELAY_US);
}

static void tm1637_dio(tm1637 *self, int level)
{
    gpio_set_level(self->dio, level);
    ets_delay_us(TM1637_BIT_DELAY_US);
}

static void tm1637_start(tm1637 *self)
{
    /* DIO falls while CLK is high. */
    tm1637_dio(self, 1);
    tm1637_clk(self, 1);
    tm1637_dio(self, 0);
    tm1637_clk(self, 0);
}

static void tm1637_stop(tm1637 *self)
{
    /* DIO rises while CLK is high. */
    tm1637_clk(self, 0);
    tm1637_dio(self, 0);
    tm1637_clk(self, 1);
    tm1637_dio(self, 1);
}

/* Shift one byte LSB-first, then clock the ACK bit. The chip pulls DIO low to
 * ACK; we release DIO during that clock so it can drive it. */
static void tm1637_write_byte(tm1637 *self, uint8_t b)
{
    for (uint8_t i = 0; i < 8u; i++) {
        tm1637_clk(self, 0);
        tm1637_dio(self, (b & 0x01u) ? 1 : 0);
        b = (uint8_t)(b >> 1);
        tm1637_clk(self, 1);
    }

    /* ACK: release DIO, pulse CLK, sample, then take DIO low again. */
    tm1637_clk(self, 0);
    tm1637_dio(self, 1);
    tm1637_clk(self, 1);
    (void)gpio_get_level(self->dio);   /* ACK bit (low = acknowledged) */
    tm1637_clk(self, 0);
    tm1637_dio(self, 0);
}

esp_err_t tm1637_init(tm1637 *self, gpio_num_t clk, gpio_num_t dio, uint8_t brightness)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->clk        = clk;
    self->dio        = dio;
    self->brightness = (brightness > 7u) ? 7u : brightness;

    gpio_set_direction(clk, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(dio, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(clk, 1);
    gpio_set_level(dio, 1);
    return ESP_OK;
}

esp_err_t tm1637_show(tm1637 *self, const uint8_t segments[4])
{
    if (self == NULL || segments == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 1) Data command: write to display register, auto-increment address. */
    tm1637_start(self);
    tm1637_write_byte(self, 0x40u);
    tm1637_stop(self);

    /* 2) Address command 0xC0 (digit 0) + four data bytes, auto-incremented. */
    tm1637_start(self);
    tm1637_write_byte(self, 0xC0u);
    for (uint8_t i = 0; i < 4u; i++) {
        tm1637_write_byte(self, segments[i]);
    }
    tm1637_stop(self);

    /* 3) Display control: on + brightness. */
    tm1637_start(self);
    tm1637_write_byte(self, (uint8_t)(0x88u | (self->brightness & 0x07u)));
    tm1637_stop(self);

    return ESP_OK;
}
