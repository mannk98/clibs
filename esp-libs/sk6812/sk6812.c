#include "sk6812.h"

/* Scale a channel by brightness/255. The uint16 product is <= 255*255 = 65025,
 * so there is no overflow and no UB (no negative shifts). */
static uint8_t scale(uint8_t c, uint8_t brightness) {
    return (uint8_t)(((uint16_t)c * (uint16_t)brightness) / 255u);
}

size_t sk6812_render(const sk6812_rgbw *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size) {
    if (!px || !out || n == 0) {
        return 0;
    }
    size_t need = n * 4u;
    if (out_size < need) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        out[i * 4 + 0] = scale(px[i].g, brightness);   /* G */
        out[i * 4 + 1] = scale(px[i].r, brightness);   /* R */
        out[i * 4 + 2] = scale(px[i].b, brightness);   /* B */
        out[i * 4 + 3] = scale(px[i].w, brightness);   /* W */
    }
    return need;
}
