#include "ssd1306_gfx.h"

void ssd1306_draw_hline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, bool on)
{
    if (!fb || w == 0u) {
        return;
    }
    for (uint8_t i = 0; i < w; i++) {
        ssd1306_fb_set_pixel(fb, (uint8_t)(x + i), y, on);
    }
}

void ssd1306_draw_vline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t h, bool on)
{
    if (!fb || h == 0u) {
        return;
    }
    for (uint8_t i = 0; i < h; i++) {
        ssd1306_fb_set_pixel(fb, x, (uint8_t)(y + i), on);
    }
}

void ssd1306_draw_line(ssd1306_fb *fb, uint8_t x0, uint8_t y0,
                       uint8_t x1, uint8_t y1, bool on)
{
    if (!fb) {
        return;
    }

    /* Integer Bresenham over signed ints — no left-shift of a negative value. */
    int x = (int)x0;
    int y = (int)y0;
    int xe = (int)x1;
    int ye = (int)y1;

    int dx = xe - x;
    if (dx < 0) {
        dx = -dx;
    }
    int dy = ye - y;
    if (dy < 0) {
        dy = -dy;
    }
    int sx = (x < xe) ? 1 : -1;
    int sy = (y < ye) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        ssd1306_fb_set_pixel(fb, (uint8_t)x, (uint8_t)y, on);
        if (x == xe && y == ye) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void ssd1306_draw_rect(ssd1306_fb *fb, uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h, bool on)
{
    if (!fb || w == 0u || h == 0u) {
        return;
    }
    /* Top + bottom edges (full width), left + right edges (full height). The
     * corners overlap, which is harmless since set_pixel is idempotent. */
    ssd1306_draw_hline(fb, x, y, w, on);
    ssd1306_draw_hline(fb, x, (uint8_t)(y + h - 1u), w, on);
    ssd1306_draw_vline(fb, x, y, h, on);
    ssd1306_draw_vline(fb, (uint8_t)(x + w - 1u), y, h, on);
}

void ssd1306_fill_rect(ssd1306_fb *fb, uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h, bool on)
{
    if (!fb || w == 0u || h == 0u) {
        return;
    }
    for (uint8_t row = 0; row < h; row++) {
        ssd1306_draw_hline(fb, x, (uint8_t)(y + row), w, on);
    }
}
