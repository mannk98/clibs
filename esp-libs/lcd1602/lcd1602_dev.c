#include "lcd1602_dev.h"
#include "i2c_bus.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* EN-pulse / settle delays. FIRST CUT — the HD44780 needs the E line held
 * >=450ns high and a few us between nibbles, and >=1.5ms after clear/home.
 * ets_delay_us is coarse; tune on real PCF8574+HD44780 hardware. */
#define LCD1602_EN_US   1u    /* settle between the four PCF8574 latch bytes  */
#define LCD1602_CMD_US  50u   /* most commands complete in ~37us             */
#define LCD1602_LONG_US 2000u /* clear-display / return-home (~1.52ms)        */

/* Clock one HD44780 byte: frame it into four PCF8574 bytes (EN-high/EN-low per
 * nibble) and stream them over i2c_bus with a settle delay after each byte. */
static esp_err_t lcd1602_write_byte(lcd1602 *self, uint8_t value, bool rs)
{
    uint8_t seq[4];
    lcd1602_encode_byte(value, rs, self->backlight, seq);
    for (int i = 0; i < 4; i++) {
        esp_err_t e = i2c_bus_write(self->addr, &seq[i], 1);
        if (e != ESP_OK) {
            return e;
        }
        ets_delay_us(LCD1602_EN_US);
    }
    return ESP_OK;
}

esp_err_t lcd1602_init(lcd1602 *self, uint8_t addr)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    self->backlight = true;
    /* HD44780 4-bit init: wake to 8-bit thrice (0x33,0x32 fold the canonical
     * 0x30.../0x20 nibbles), then 4-bit/2-line, display-on, entry-mode, clear. */
    static const uint8_t init_seq[] = { 0x33u, 0x32u, 0x28u, 0x0Cu, 0x06u, 0x01u };
    for (size_t i = 0; i < sizeof init_seq; i++) {
        esp_err_t e = lcd1602_write_byte(self, init_seq[i], false);
        if (e != ESP_OK) {
            return e;
        }
        ets_delay_us(init_seq[i] == 0x01u ? LCD1602_LONG_US : LCD1602_CMD_US);
    }
    return ESP_OK;
}

esp_err_t lcd1602_command(lcd1602 *self, uint8_t cmd)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t e = lcd1602_write_byte(self, cmd, false);
    if (e != ESP_OK) {
        return e;
    }
    /* clear-display (0x01) and return-home (0x02) need the long settle. */
    ets_delay_us((cmd == 0x01u || cmd == 0x02u) ? LCD1602_LONG_US : LCD1602_CMD_US);
    return ESP_OK;
}

esp_err_t lcd1602_putc(lcd1602 *self, char c)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t e = lcd1602_write_byte(self, (uint8_t)c, true);
    if (e != ESP_OK) {
        return e;
    }
    ets_delay_us(LCD1602_CMD_US);
    return ESP_OK;
}

esp_err_t lcd1602_puts(lcd1602 *self, const char *s)
{
    if (!self || !s) {
        return ESP_ERR_INVALID_ARG;
    }
    for (; *s; s++) {
        esp_err_t e = lcd1602_putc(self, *s);
        if (e != ESP_OK) {
            return e;
        }
    }
    return ESP_OK;
}

esp_err_t lcd1602_set_cursor(lcd1602 *self, uint8_t col, uint8_t row)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t addr = (uint8_t)((row ? 0x40u : 0u) + col);
    return lcd1602_command(self, (uint8_t)(0x80u | addr));
}
