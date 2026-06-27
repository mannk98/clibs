/* Host Unity test for the pure ssd1306 5x7 text layer (RED step).
 * Tests the API contract only — ssd1306_text.h / .c do not exist yet.
 * Renders into the cycle-11 framebuffer (ssd1306_fb) and reconstructs glyph
 * column bytes via get_pixel to assert against the bundled 5x7 font. */
#include "unity.h"
#include "ssd1306_fb.h"
#include "ssd1306_text.h"

void setUp(void) {}
void tearDown(void) {}

/* 128x64 -> SSD1306_FB_BYTES(128,64) == 1024 bytes. */
static uint8_t g_buf[SSD1306_FB_BYTES(128, 64)];

/* Reconstruct the column byte at framebuffer column (x) for a glyph drawn at
 * origin (ox,oy): bit r (0..6) == pixel (x, oy+r). Mirrors the font's
 * column-major, bit0 = top row encoding. */
static uint8_t recon_col(const ssd1306_fb *fb, uint8_t x, uint8_t oy)
{
    uint8_t v = 0;
    for (uint8_t r = 0; r < 7; r++) {
        if (ssd1306_fb_get_pixel(fb, x, (uint8_t)(oy + r))) {
            v |= (uint8_t)(1u << r);
        }
    }
    return v;
}

/* Bullet 1: draw_char(' ') leaves every reconstructed column byte == 0x00. */
static void test_ssd1306_text_draw_space_is_blank(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_char(&fb, 0, 0, ' ');
    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, recon_col(&fb, col, 0));
    }
}

/* Bullet 2: draw_char('A') -> reconstructed columns == {0x7C,0x12,0x11,0x12,0x7C}. */
static void test_ssd1306_text_draw_char_A_glyph(void)
{
    static const uint8_t expect_A[5] = {0x7C, 0x12, 0x11, 0x12, 0x7C};
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_char(&fb, 0, 0, 'A');
    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(expect_A[col], recon_col(&fb, col, 0));
    }
}

/* Bullet 3a: draw_string("A") returns end x == 6 (5px glyph + 1px gap). */
static void test_ssd1306_text_draw_string_single_returns_6(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    uint8_t end = ssd1306_draw_string(&fb, 0, 0, "A");
    TEST_ASSERT_EQUAL_UINT8(6, end);
}

/* Bullet 3b: draw_string("AA") returns 12 and renders 'A' at both x=0 and x=6. */
static void test_ssd1306_text_draw_string_two_chars(void)
{
    static const uint8_t expect_A[5] = {0x7C, 0x12, 0x11, 0x12, 0x7C};
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    uint8_t end = ssd1306_draw_string(&fb, 0, 0, "AA");
    TEST_ASSERT_EQUAL_UINT8(12, end);

    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(expect_A[col], recon_col(&fb, col, 0));            /* first 'A' at x=0 */
        TEST_ASSERT_EQUAL_HEX8(expect_A[col], recon_col(&fb, (uint8_t)(6 + col), 0)); /* second 'A' at x=6 */
    }
}

/* Bullet 4: an out-of-range / unknown char renders a blank space (no pixels set).
 * 0x7F (DEL, just past the 0x20..0x7E table) and a high-bit byte both map to blank. */
static void test_ssd1306_text_unknown_char_is_blank(void)
{
    ssd1306_fb fb;

    ssd1306_fb_init(&fb, g_buf, 128, 64);
    ssd1306_draw_char(&fb, 0, 0, (char)0x7F);
    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, recon_col(&fb, col, 0));
    }

    ssd1306_fb_init(&fb, g_buf, 128, 64);
    ssd1306_draw_char(&fb, 0, 0, (char)0x80);
    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, recon_col(&fb, col, 0));
    }
}

/* Bullet 5: NULL fb -> draw_char is a no-op (no crash); draw_string(NULL,5,..)
 * and draw_string(fb,..,NULL) return x unchanged. */
static void test_ssd1306_text_null_guards(void)
{
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    ssd1306_draw_char(NULL, 0, 0, 'A');                       /* must not crash */

    TEST_ASSERT_EQUAL_UINT8(5, ssd1306_draw_string(NULL, 5, 0, "A"));  /* NULL fb -> x unchanged */
    TEST_ASSERT_EQUAL_UINT8(9, ssd1306_draw_string(&fb, 9, 0, NULL));  /* NULL s  -> x unchanged */
}

/* Bullet 6 (UB-freedom) is enforced by the sanitize gate (UBSan+ASan) over this
 * same pure suite; recon_col here also exercises (1u<<row) on uint8 operands, so
 * any signed-shift UB in the impl surfaces under -fsanitize=undefined. The 'A'
 * glyph round-trip below makes the suite non-vacuous under the sanitizer. */
static void test_ssd1306_text_glyph_roundtrip_no_ub(void)
{
    static const uint8_t expect_A[5] = {0x7C, 0x12, 0x11, 0x12, 0x7C};
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, g_buf, 128, 64);

    uint8_t end = ssd1306_draw_string(&fb, 10, 16, "A");
    TEST_ASSERT_EQUAL_UINT8(16, end);
    for (uint8_t col = 0; col < 5; col++) {
        TEST_ASSERT_EQUAL_HEX8(expect_A[col], recon_col(&fb, (uint8_t)(10 + col), 16));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ssd1306_text_draw_space_is_blank);
    RUN_TEST(test_ssd1306_text_draw_char_A_glyph);
    RUN_TEST(test_ssd1306_text_draw_string_single_returns_6);
    RUN_TEST(test_ssd1306_text_draw_string_two_chars);
    RUN_TEST(test_ssd1306_text_unknown_char_is_blank);
    RUN_TEST(test_ssd1306_text_null_guards);
    RUN_TEST(test_ssd1306_text_glyph_roundtrip_no_ub);
    return UNITY_END();
}
