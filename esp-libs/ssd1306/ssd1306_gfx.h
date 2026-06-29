#ifndef CLIBS_SSD1306_GFX_H
#define CLIBS_SSD1306_GFX_H

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306_fb.h"

/* Pure graphics primitives for the SSD1306 framebuffer (no SDK glue).
 * Every pixel is written through ssd1306_fb_set_pixel, so off-buffer writes
 * clip silently. NULL fb -> no-op; zero-extent shapes (w==0 / h==0) -> no-op.
 * Not reentrant. */

/* Horizontal run of w pixels starting at (x,y): lights x .. x+w-1 at row y.
 * NULL fb or w==0 -> no-op. */
void ssd1306_draw_hline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, bool on);

/* Vertical run of h pixels starting at (x,y): lights y .. y+h-1 at column x.
 * NULL fb or h==0 -> no-op. */
void ssd1306_draw_vline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t h, bool on);

/* Integer Bresenham line from (x0,y0) to (x1,y1) inclusive. NULL fb -> no-op. */
void ssd1306_draw_line(ssd1306_fb *fb, uint8_t x0, uint8_t y0,
                       uint8_t x1, uint8_t y1, bool on);

/* Outline of the w x h rectangle whose top-left is (x,y) (4 edges, hollow).
 * NULL fb or w==0 or h==0 -> no-op. */
void ssd1306_draw_rect(ssd1306_fb *fb, uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h, bool on);

/* Solid w x h rectangle whose top-left is (x,y). NULL fb or w==0 or h==0
 * -> no-op. */
void ssd1306_fill_rect(ssd1306_fb *fb, uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h, bool on);

#endif /* CLIBS_SSD1306_GFX_H */
