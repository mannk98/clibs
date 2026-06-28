/* Host Unity test for the pure ssd1306 graphics-primitives layer (RED step).
 * Tests the API contract only — ssd1306_gfx.h / .c do not exist yet.
 * Draws into the cycle-11 framebuffer (ssd1306_fb) and asserts pixel state via
 * ssd1306_fb_get_pixel. Every primitive must write through ssd1306_fb_set_pixel
 * (so off-buffer writes clip) and must be a no-op for NULL fb / w==0 / h==0.
 * Host suite links: test_ssd1306_gfx.c + ssd1306_gfx.c + ssd1306_fb.c. */
#include "unity.h"
#include "ssd1306_fb.h"
#include "ssd1306_gfx.h"

void setUp(void) {}
void tearDown(void) {}

/* 128x64 -> SSD1306_FB_BYTES(128,64) == 1024 bytes. */
static uint8_t g_buf[SSD1306_FB_BYTES(128, 64)];

/* Bullet 1: ssd1306_draw_hline(&fb,2,3,4,true) lights x in [2..5] at y=3;
 * neighbours (1,3) and (6,3) stay off. */
static void test_ssd1306_gfx_hline_spans_w_pixels(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_hline(&fb, 2, 3, 4, true);

    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 2, 3));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 3));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 4, 3));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 5, 3));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 1, 3));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 6, 3));
}

/* Bullet 2: ssd1306_draw_vline(&fb,3,2,4,true) lights y in [2..5] at x=3;
 * (3,6) stays off. */
static void test_ssd1306_gfx_vline_spans_h_pixels(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_vline(&fb, 3, 2, 4, true);

    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 2));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 3));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 4));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 5));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 3, 6));
}

/* Bullet 3: ssd1306_draw_line(&fb,0,0,3,3,true) is the integer diagonal
 * (0,0),(1,1),(2,2),(3,3) on; the off-diagonal (0,1) stays off. */
static void test_ssd1306_gfx_line_diagonal_bresenham(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_line(&fb, 0, 0, 3, 3, true);

    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 1, 1));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 2, 2));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 3));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 1));
}

/* Bullet 4: ssd1306_draw_rect(&fb,0,0,4,3,true) draws only the outline:
 * corners (0,0),(3,0),(0,2),(3,2) on; interior (1,1) off. */
static void test_ssd1306_gfx_rect_outline_only(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_rect(&fb, 0, 0, 4, 3, true);

    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 2));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 2));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 1, 1));
}

/* Bullet 5: ssd1306_fill_rect(&fb,0,0,2,2,true) lights the full 2x2 block. */
static void test_ssd1306_gfx_fill_rect_solid(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fill_rect(&fb, 0, 0, 2, 2, true);

    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 1, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 1));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 1, 1));
}

/* Bullet 6: fb==NULL -> every primitive is a no-op and must not crash. */
static void test_ssd1306_gfx_null_fb_is_noop(void)
{
    ssd1306_draw_hline(NULL, 2, 3, 4, true);
    ssd1306_draw_vline(NULL, 3, 2, 4, true);
    ssd1306_draw_line(NULL, 0, 0, 3, 3, true);
    ssd1306_draw_rect(NULL, 0, 0, 4, 3, true);
    ssd1306_fill_rect(NULL, 0, 0, 2, 2, true);
    TEST_PASS();  /* reached here without a crash */
}

/* Bullet 7: w==0 or h==0 -> ssd1306_draw_rect / ssd1306_fill_rect is a no-op
 * (no pixel at the origin is touched). */
static void test_ssd1306_gfx_zero_dim_rect_is_noop(void)
{
    ssd1306_fb fb;

    ssd1306_fb_init(&fb, g_buf, 128, 64);
    ssd1306_draw_rect(&fb, 0, 0, 0, 3, true);
    ssd1306_draw_rect(&fb, 0, 0, 4, 0, true);
    ssd1306_fill_rect(&fb, 0, 0, 0, 2, true);
    ssd1306_fill_rect(&fb, 0, 0, 2, 0, true);

    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ssd1306_gfx_hline_spans_w_pixels);
    RUN_TEST(test_ssd1306_gfx_vline_spans_h_pixels);
    RUN_TEST(test_ssd1306_gfx_line_diagonal_bresenham);
    RUN_TEST(test_ssd1306_gfx_rect_outline_only);
    RUN_TEST(test_ssd1306_gfx_fill_rect_solid);
    RUN_TEST(test_ssd1306_gfx_null_fb_is_noop);
    RUN_TEST(test_ssd1306_gfx_zero_dim_rect_is_noop);
    return UNITY_END();
}
