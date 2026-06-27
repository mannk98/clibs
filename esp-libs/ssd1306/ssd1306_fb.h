#ifndef CLIBS_SSD1306_FB_H
#define CLIBS_SSD1306_FB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 1bpp page-addressed framebuffer for an SSD1306-style OLED. The byte buffer is
 * exactly the SSD1306 GDDRAM layout: idx = (y/8)*width + x, bit = y%8 (bit0 = top
 * row of the page), so the glue streams it to the panel unchanged. Fields are
 * private; use the accessors. Not reentrant. */
typedef struct {
    uint8_t *buf;
    uint8_t width;
    uint8_t height;
} ssd1306_fb;

/* Bytes the caller must reserve for a width x height framebuffer (1 bit/pixel). */
#define SSD1306_FB_BYTES(w, h) ((size_t)(w) * (h) / 8u)

/* Bind self to caller-owned buf (>= SSD1306_FB_BYTES(width,height)) and clear it.
 * NULL self -> no-op. */
void ssd1306_fb_init(ssd1306_fb *self, uint8_t *buf, uint8_t width, uint8_t height);

/* Set pixel (x,y) on/off. Out-of-range coords or NULL self/buf -> no-op. */
void ssd1306_fb_set_pixel(ssd1306_fb *self, uint8_t x, uint8_t y, bool on);

/* Read pixel (x,y). Out-of-range coords or NULL self/buf -> false. */
bool ssd1306_fb_get_pixel(const ssd1306_fb *self, uint8_t x, uint8_t y);

/* Set every pixel on (true) or off (false). NULL self/buf -> no-op. */
void ssd1306_fb_fill(ssd1306_fb *self, bool on);

/* Clear every pixel (equivalent to fill(false)). NULL self/buf -> no-op. */
void ssd1306_fb_clear(ssd1306_fb *self);

#endif /* CLIBS_SSD1306_FB_H */
