#include "ssd1306_fb.h"

void ssd1306_fb_clear(ssd1306_fb *self)
{
    if (!self || !self->buf) {
        return;
    }
    size_t n = SSD1306_FB_BYTES(self->width, self->height);
    for (size_t i = 0; i < n; i++) {
        self->buf[i] = 0u;
    }
}

void ssd1306_fb_init(ssd1306_fb *self, uint8_t *buf, uint8_t width, uint8_t height)
{
    if (!self) {
        return;
    }
    self->buf = buf;
    self->width = width;
    self->height = height;
    ssd1306_fb_clear(self);
}

void ssd1306_fb_set_pixel(ssd1306_fb *self, uint8_t x, uint8_t y, bool on)
{
    if (!self || !self->buf || x >= self->width || y >= self->height) {
        return;
    }
    size_t idx = (size_t)(y >> 3) * self->width + x;
    uint8_t mask = (uint8_t)(1u << (y & 7u));
    if (on) {
        self->buf[idx] = (uint8_t)(self->buf[idx] | mask);
    } else {
        self->buf[idx] = (uint8_t)(self->buf[idx] & (uint8_t)(~mask));
    }
}

bool ssd1306_fb_get_pixel(const ssd1306_fb *self, uint8_t x, uint8_t y)
{
    if (!self || !self->buf || x >= self->width || y >= self->height) {
        return false;
    }
    size_t idx = (size_t)(y >> 3) * self->width + x;
    return (((uint8_t)(self->buf[idx] >> (y & 7u))) & 1u) != 0u;
}

void ssd1306_fb_fill(ssd1306_fb *self, bool on)
{
    if (!self || !self->buf) {
        return;
    }
    size_t n = SSD1306_FB_BYTES(self->width, self->height);
    uint8_t v = on ? 0xFFu : 0x00u;
    for (size_t i = 0; i < n; i++) {
        self->buf[i] = v;
    }
}
