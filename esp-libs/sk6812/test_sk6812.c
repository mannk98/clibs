/* Host Unity test for the pure sk6812 RGBW render part (L3 driver, header-split).
 * Tests the API contract only — sk6812.{h,c} do not exist yet (RED step).
 * Do NOT test the SDK glue (sk6812_dev.{h,c}); it is COMPILE_GATE deferred (xtensa).
 *
 * Contract under test (story C14-1):
 *   typedef struct { uint8_t r,g,b,w; } sk6812_rgbw;
 *   size_t sk6812_render(const sk6812_rgbw *px, size_t n, uint8_t brightness,
 *                        uint8_t *out, size_t out_size);
 *   GRBW order: out[4i]=G, +1=R, +2=B, +3=W, each scaled (uint16_t)c*brightness/255.
 *   need = 4*n; returns need, or 0 on NULL px/out, n==0, out_size<need. */
#include "unity.h"
#include "sk6812.h"

void setUp(void) {}
void tearDown(void) {}

/* px {r10,g20,b30,w40} n=1 brightness=255 out[4] ->
 * returns 4 and out = {20,10,30,40} (GRBW, full brightness = identity). */
static void test_sk6812_render_single_full_brightness_grbw(void)
{
    const sk6812_rgbw px[1] = { { 10, 20, 30, 40 } };
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };

    size_t written = sk6812_render(px, 1, 255, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(4, written);
    TEST_ASSERT_EQUAL_UINT8(20, out[0]); /* G */
    TEST_ASSERT_EQUAL_UINT8(10, out[1]); /* R */
    TEST_ASSERT_EQUAL_UINT8(30, out[2]); /* B */
    TEST_ASSERT_EQUAL_UINT8(40, out[3]); /* W */
}

/* px {r10,g20,b30,w40} n=1 brightness=128 out[4] ->
 * returns 4 and out = {10,5,15,20}:
 *   G 20*128/255=10, R 10*128/255=5, B 30*128/255=15, W 40*128/255=20. */
static void test_sk6812_render_half_brightness_scales_each_channel(void)
{
    const sk6812_rgbw px[1] = { { 10, 20, 30, 40 } };
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };

    size_t written = sk6812_render(px, 1, 128, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(4, written);
    TEST_ASSERT_EQUAL_UINT8(10, out[0]); /* G = 20*128/255 = 10 */
    TEST_ASSERT_EQUAL_UINT8(5,  out[1]); /* R = 10*128/255 = 5  */
    TEST_ASSERT_EQUAL_UINT8(15, out[2]); /* B = 30*128/255 = 15 */
    TEST_ASSERT_EQUAL_UINT8(20, out[3]); /* W = 40*128/255 = 20 */
}

/* out_size 3 (< need 4) for n=1 -> returns 0. */
static void test_sk6812_render_out_too_small_returns_zero(void)
{
    const sk6812_rgbw px[1] = { { 10, 20, 30, 40 } };
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, sk6812_render(px, 1, 255, out, 3));
}

/* NULL px -> returns 0. */
static void test_sk6812_render_null_px_returns_zero(void)
{
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, sk6812_render(NULL, 1, 255, out, sizeof out));
}

/* NULL out -> returns 0. */
static void test_sk6812_render_null_out_returns_zero(void)
{
    const sk6812_rgbw px[1] = { { 10, 20, 30, 40 } };

    TEST_ASSERT_EQUAL_size_t(0, sk6812_render(px, 1, 255, NULL, 4));
}

/* n 0 -> returns 0. */
static void test_sk6812_render_zero_pixels_returns_zero(void)
{
    const sk6812_rgbw px[1] = { { 10, 20, 30, 40 } };
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, sk6812_render(px, 0, 255, out, sizeof out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sk6812_render_single_full_brightness_grbw);
    RUN_TEST(test_sk6812_render_half_brightness_scales_each_channel);
    RUN_TEST(test_sk6812_render_out_too_small_returns_zero);
    RUN_TEST(test_sk6812_render_null_px_returns_zero);
    RUN_TEST(test_sk6812_render_null_out_returns_zero);
    RUN_TEST(test_sk6812_render_zero_pixels_returns_zero);
    return UNITY_END();
}
