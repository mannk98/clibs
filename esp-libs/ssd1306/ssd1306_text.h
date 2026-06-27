#ifndef CLIBS_SSD1306_TEXT_H
#define CLIBS_SSD1306_TEXT_H

#include <stdint.h>
#include "ssd1306_fb.h"

/* Pure 5x7 ASCII text layer for the SSD1306 framebuffer (no SDK glue).
 * Glyphs are column-major: each printable character occupies 5 column bytes,
 * bit0 = top row. Characters advance 6px (5px glyph + 1px gap). The bundled
 * font covers ASCII 0x20..0x7E; anything else renders as a blank space. */

/* Draw glyph c at framebuffer origin (x,y) into fb. An unknown / out-of-range
 * character renders as a blank space (no pixels set). NULL fb -> no-op. */
void ssd1306_draw_char(ssd1306_fb *fb, uint8_t x, uint8_t y, char c);

/* Draw string s starting at (x,y), advancing 6px per character (5px glyph +
 * 1px gap). Returns the end x (one past the last advance). NULL fb or NULL s
 * -> returns x unchanged. */
uint8_t ssd1306_draw_string(ssd1306_fb *fb, uint8_t x, uint8_t y, const char *s);

#endif /* CLIBS_SSD1306_TEXT_H */
