/* Host Unity test for the pure ws2812_render part (L3 driver, header-split).
 * Tests the API contract only — ws2812_render.{h,c} do not exist yet (RED step).
 * Do NOT test the SDK glue (ws2812.{h,c}); it is COMPILE_GATE deferred (xtensa). */
#include "unity.h"
#include "ws2812_render.h"

void setUp(void) {}
void tearDown(void) {}

/* render({r255,g128,b0}, 1, 255, out, 3) -> returns 3, out = {128,255,0} (GRB). */
static void test_ws2812_render_single_full_brightness_grb(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    size_t written = ws2812_render(px, 1, 255, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(3, written);
    TEST_ASSERT_EQUAL_UINT8(128, out[0]); /* G */
    TEST_ASSERT_EQUAL_UINT8(255, out[1]); /* R */
    TEST_ASSERT_EQUAL_UINT8(0,   out[2]); /* B */
}

/* render({r255,g128,b0}, 1, 128, out, 3) -> out = {64,128,0}
 * (g 128*128/255=64, r 255*128/255=128, b 0). */
static void test_ws2812_render_half_brightness_scales_each_channel(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    size_t written = ws2812_render(px, 1, 128, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(3, written);
    TEST_ASSERT_EQUAL_UINT8(64,  out[0]); /* G = 128*128/255 = 64 */
    TEST_ASSERT_EQUAL_UINT8(128, out[1]); /* R = 255*128/255 = 128 */
    TEST_ASSERT_EQUAL_UINT8(0,   out[2]); /* B */
}

/* render({r255,g128,b0}, 1, 0, out, 3) -> out = {0,0,0}. */
static void test_ws2812_render_zero_brightness_all_off(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    size_t written = ws2812_render(px, 1, 0, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(3, written);
    TEST_ASSERT_EQUAL_UINT8(0, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0, out[2]);
}

/* render(px[2]={(255,0,0),(0,0,255)}, 2, 255, out, 6) -> returns 6,
 * out = {0,255,0, 0,0,255} (per-pixel GRB). */
static void test_ws2812_render_two_pixels(void)
{
    const ws2812_rgb px[2] = { { 255, 0, 0 }, { 0, 0, 255 } };
    uint8_t out[6] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

    size_t written = ws2812_render(px, 2, 255, out, sizeof out);

    TEST_ASSERT_EQUAL_size_t(6, written);
    /* pixel 0 = red -> G,R,B = 0,255,0 */
    TEST_ASSERT_EQUAL_UINT8(0,   out[0]);
    TEST_ASSERT_EQUAL_UINT8(255, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0,   out[2]);
    /* pixel 1 = blue -> G,R,B = 0,0,255 */
    TEST_ASSERT_EQUAL_UINT8(0,   out[3]);
    TEST_ASSERT_EQUAL_UINT8(0,   out[4]);
    TEST_ASSERT_EQUAL_UINT8(255, out[5]);
}

/* out_size 2 for n 1 (need 3) -> returns 0. */
static void test_ws2812_render_out_too_small_returns_zero(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, ws2812_render(px, 1, 255, out, 2));
}

/* n 0 -> returns 0. */
static void test_ws2812_render_zero_pixels_returns_zero(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, ws2812_render(px, 0, 255, out, sizeof out));
}

/* NULL px / NULL out -> returns 0. */
static void test_ws2812_render_null_args_return_zero(void)
{
    const ws2812_rgb px[1] = { { 255, 128, 0 } };
    uint8_t out[3] = { 0xAA, 0xAA, 0xAA };

    TEST_ASSERT_EQUAL_size_t(0, ws2812_render(NULL, 1, 255, out, sizeof out));
    TEST_ASSERT_EQUAL_size_t(0, ws2812_render(px,   1, 255, NULL, sizeof out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ws2812_render_single_full_brightness_grb);
    RUN_TEST(test_ws2812_render_half_brightness_scales_each_channel);
    RUN_TEST(test_ws2812_render_zero_brightness_all_off);
    RUN_TEST(test_ws2812_render_two_pixels);
    RUN_TEST(test_ws2812_render_out_too_small_returns_zero);
    RUN_TEST(test_ws2812_render_zero_pixels_returns_zero);
    RUN_TEST(test_ws2812_render_null_args_return_zero);
    return UNITY_END();
}
