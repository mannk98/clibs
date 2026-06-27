#ifndef CLIBS_SK6812_H
#define CLIBS_SK6812_H

#include <stdint.h>
#include <stddef.h>

/* One pixel, as the caller supplies it (R, G, B, W). */
typedef struct { uint8_t r, g, b, w; } sk6812_rgbw;

/* Render n pixels into an SK6812 RGBW (GRBW wire order) byte buffer:
 * out[4i]=G, out[4i+1]=R, out[4i+2]=B, out[4i+3]=W, each channel scaled
 * (uint16_t)c*brightness/255. out_size must be >= 4*n.
 * Returns 4*n, or 0 if px or out is NULL, n == 0, or out_size < 4*n.
 * Pure, freestanding, host-testable. */
size_t sk6812_render(const sk6812_rgbw *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size);

#endif /* CLIBS_SK6812_H */
