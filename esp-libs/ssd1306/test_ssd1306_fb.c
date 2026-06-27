/* Host Unity test for the pure ssd1306_fb framebuffer (RED step).
 * Tests the API contract only — ssd1306_fb.h / .c do not exist yet. */
#include "unity.h"
#include "ssd1306_fb.h"

void setUp(void) {}
void tearDown(void) {}

/* 128x64 -> SSD1306_FB_BYTES(128,64) == 128*64/8 == 1024 bytes. */
static uint8_t g_buf[SSD1306_FB_BYTES(128, 64)];

/* Bullet 1: init -> buffer sized 1024 and every pixel reads false. */
static void test_ssd1306_init_all_pixels_clear(void)
{
    ssd1306_fb fb;
    TEST_ASSERT_EQUAL_size_t(1024u, SSD1306_FB_BYTES(128, 64));

    ssd1306_fb_init(&fb, g_buf, 128, 64);
    for (uint8_t x = 0; x < 128; x++) {
        for (uint8_t y = 0; y < 64; y++) {
            TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, x, y));
        }
    }
}

/* Bullet 2: set(0,0,true) -> buf[0]==0x01, get(0,0) true. */
static void test_ssd1306_set_origin(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 0, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_buf[0]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
}

/* Bullet 3: after (0,0), set(0,7,true) -> buf[0]==0x81 (bit0 + bit7). */
static void test_ssd1306_set_top_and_bottom_of_page(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 0, 0, true);
    ssd1306_fb_set_pixel(&fb, 0, 7, true);
    TEST_ASSERT_EQUAL_HEX8(0x81, g_buf[0]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 7));
}

/* Bullet 4: from buf[0]==0x81, set(0,0,false) -> buf[0]==0x80 (only bit7 left). */
static void test_ssd1306_clear_one_pixel(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 0, 0, true);
    ssd1306_fb_set_pixel(&fb, 0, 7, true);
    ssd1306_fb_set_pixel(&fb, 0, 0, false);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_buf[0]);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 7));
}

/* Bullet 5: set(0,8,true) -> page 1, idx == (8/8)*128 + 0 == 128, buf[128]==0x01. */
static void test_ssd1306_set_page1(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 0, 8, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_buf[128]);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_buf[0]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 8));
}

/* Bullet 6: set(127,63,true) -> idx == (63/8)*128 + 127 == 1023, bit == 63%8 == 7 -> 0x80. */
static void test_ssd1306_set_last_pixel(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 127, 63, true);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_buf[1023]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 127, 63));
}

/* Bullet 7: fill(true) -> buf[0]==0xFF and get(50,50) true; clear -> buf[0]==0x00, get false. */
static void test_ssd1306_fill_and_clear(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_fill(&fb, true);
    TEST_ASSERT_EQUAL_HEX8(0xFF, g_buf[0]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 50, 50));

    ssd1306_fb_clear(&fb);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_buf[0]);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 50, 50));
}

/* Bullet 8: OOB set(128,0)/(0,64) are no-ops -> get false, buf[0] untouched (0x00). */
static void test_ssd1306_out_of_bounds_noop(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_fb_set_pixel(&fb, 128, 0, true);   /* x == width  -> OOB */
    ssd1306_fb_set_pixel(&fb, 0, 64, true);    /* y == height -> OOB */
    TEST_ASSERT_EQUAL_HEX8(0x00, g_buf[0]);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 128, 0));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 64));
}

/* Bullet 9: NULL self -> set is a no-op, get returns false. */
static void test_ssd1306_null_self(void)
{
    ssd1306_fb_set_pixel(NULL, 0, 0, true);   /* must not crash */
    ssd1306_fb_fill(NULL, true);              /* must not crash */
    ssd1306_fb_clear(NULL);                   /* must not crash */
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(NULL, 0, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ssd1306_init_all_pixels_clear);
    RUN_TEST(test_ssd1306_set_origin);
    RUN_TEST(test_ssd1306_set_top_and_bottom_of_page);
    RUN_TEST(test_ssd1306_clear_one_pixel);
    RUN_TEST(test_ssd1306_set_page1);
    RUN_TEST(test_ssd1306_set_last_pixel);
    RUN_TEST(test_ssd1306_fill_and_clear);
    RUN_TEST(test_ssd1306_out_of_bounds_noop);
    RUN_TEST(test_ssd1306_null_self);
    return UNITY_END();
}
