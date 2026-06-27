#ifndef CLIBS_WS2812_RENDER_H
#define CLIBS_WS2812_RENDER_H

#include <stdint.h>
#include <stddef.h>

/* One pixel, as the caller supplies it (R, G, B). */
typedef struct { uint8_t r, g, b; } ws2812_rgb;

/* Render n pixels into a WS2812 GRB byte buffer: out[3i]=G, out[3i+1]=R,
 * out[3i+2]=B, each channel scaled by brightness/255. out_size must be >= 3*n.
 * Returns 3*n, or 0 if out_size < 3*n / NULL px or out / n == 0. Pure, host-testable. */
size_t ws2812_render(const ws2812_rgb *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size);

#endif /* CLIBS_WS2812_RENDER_H */
