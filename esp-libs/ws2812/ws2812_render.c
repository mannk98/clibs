#include "ws2812_render.h"

/* Scale a channel by brightness/255. The uint16 product is <= 255*255 = 65025,
 * so there is no overflow and no UB (no negative shifts). */
static uint8_t scale(uint8_t c, uint8_t brightness) {
    return (uint8_t)(((uint16_t)c * (uint16_t)brightness) / 255u);
}

size_t ws2812_render(const ws2812_rgb *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size) {
    if (!px || !out || n == 0) {
        return 0;
    }
    size_t need = n * 3u;
    if (out_size < need) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        out[i * 3 + 0] = scale(px[i].g, brightness);   /* G */
        out[i * 3 + 1] = scale(px[i].r, brightness);   /* R */
        out[i * 3 + 2] = scale(px[i].b, brightness);   /* B */
    }
    return need;
}
