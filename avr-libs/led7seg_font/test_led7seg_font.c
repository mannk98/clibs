#include "unity.h"
#include "led7seg_font.h"

void setUp(void) {}
void tearDown(void) {}

/* Font bytes are bit-identical to the original B-macro table:
 * B11000000=0xC0 ... B10010000=0x90 (common-anode, 74HC595). */
static void test_digit_font(void) {
    TEST_ASSERT_EQUAL_HEX8(0xC0, led7seg_segments(0));
    TEST_ASSERT_EQUAL_HEX8(0xCF, led7seg_segments(1));
    TEST_ASSERT_EQUAL_HEX8(0xA4, led7seg_segments(2));
    TEST_ASSERT_EQUAL_HEX8(0xB0, led7seg_segments(3));
    TEST_ASSERT_EQUAL_HEX8(0x99, led7seg_segments(4));
    TEST_ASSERT_EQUAL_HEX8(0x92, led7seg_segments(5));
    TEST_ASSERT_EQUAL_HEX8(0x82, led7seg_segments(6));
    TEST_ASSERT_EQUAL_HEX8(0xF8, led7seg_segments(7));
    TEST_ASSERT_EQUAL_HEX8(0x80, led7seg_segments(8));
    TEST_ASSERT_EQUAL_HEX8(0x90, led7seg_segments(9));
}
static void test_out_of_range_is_off(void) {
    TEST_ASSERT_EQUAL_HEX8(0xFF, led7seg_segments(10));
    TEST_ASSERT_EQUAL_HEX8(0xFF, led7seg_segments(255));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_digit_font);
    RUN_TEST(test_out_of_range_is_off);
    return UNITY_END();
}
