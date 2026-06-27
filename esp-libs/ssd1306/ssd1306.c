#include "ssd1306.h"
#include "i2c_bus.h"

/* SSD1306 I2C control byte: 0x00 = command stream, 0x40 = data stream. */
static esp_err_t ssd1306_cmd(uint8_t addr, uint8_t c)
{
    return i2c_bus_write_reg(addr, 0x00, &c, 1);
}

esp_err_t ssd1306_init(ssd1306 *self, uint8_t addr, uint8_t *buf, uint8_t width, uint8_t height)
{
    if (!self || !buf) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    ssd1306_fb_init(&self->fb, buf, width, height);
    static const uint8_t seq[] = {
        0xAE,             /* display off */
        0x20, 0x00,       /* memory addressing mode = horizontal */
        0x40,             /* display start line 0 */
        0xA1, 0xC8,       /* segment remap + COM scan dir (typical orientation) */
        0x81, 0x7F,       /* contrast */
        0xA4,             /* resume from RAM */
        0xA6,             /* normal (non-inverted) */
        0xD5, 0x80, 0xD3, 0x00, 0xDA, 0x12, 0xD9, 0xF1, 0xDB, 0x40,
        0x8D, 0x14,       /* charge pump on */
        0xAF              /* display on */
    };
    for (size_t i = 0; i < sizeof seq; i++) {
        esp_err_t e = ssd1306_cmd(addr, seq[i]);
        if (e != ESP_OK) {
            return e;
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_show(ssd1306 *self)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t pages = (uint8_t)(self->fb.height / 8u);
    const uint8_t window[] = {
        0x21, 0x00, (uint8_t)(self->fb.width - 1),   /* column address 0..w-1 */
        0x22, 0x00, (uint8_t)(pages - 1)             /* page address 0..pages-1 */
    };
    for (size_t i = 0; i < sizeof window; i++) {
        esp_err_t e = ssd1306_cmd(self->addr, window[i]);
        if (e != ESP_OK) {
            return e;
        }
    }
    return i2c_bus_write_reg(self->addr, 0x40, self->fb.buf,
                             SSD1306_FB_BYTES(self->fb.width, self->fb.height));
}
