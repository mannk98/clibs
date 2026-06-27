/* Host Unity test for the pure hx711_parse part (L3 driver, header-split).
 * Tests the API contract only — hx711.{h,c} do not exist yet (RED step).
 * Do NOT test the SDK glue (hx711_gpio.{h,c}); it is COMPILE_GATE deferred (xtensa).
 *
 * Contract under test:
 *   int32_t hx711_parse(const uint8_t raw[3]);
 *   24-bit big-endian two's-complement (raw[0]=MSB) -> sign-extended int32.
 *   NULL -> 0. */
#include "unity.h"
#include "hx711.h"

void setUp(void) {}
void tearDown(void) {}

/* raw {0x00,0x00,0x00} -> 0. */
static void test_hx711_parse_zero_returns_zero(void)
{
    const uint8_t raw[3] = { 0x00, 0x00, 0x00 };
    TEST_ASSERT_EQUAL_INT32(0, hx711_parse(raw));
}

/* raw {0x00,0x00,0x01} -> 1. */
static void test_hx711_parse_one_returns_one(void)
{
    const uint8_t raw[3] = { 0x00, 0x00, 0x01 };
    TEST_ASSERT_EQUAL_INT32(1, hx711_parse(raw));
}

/* raw {0x7F,0xFF,0xFF} -> 8388607 (max positive 24-bit). */
static void test_hx711_parse_max_positive_returns_8388607(void)
{
    const uint8_t raw[3] = { 0x7F, 0xFF, 0xFF };
    TEST_ASSERT_EQUAL_INT32(8388607, hx711_parse(raw));
}

/* raw {0xFF,0xFF,0xFF} -> -1. */
static void test_hx711_parse_all_ones_returns_minus_one(void)
{
    const uint8_t raw[3] = { 0xFF, 0xFF, 0xFF };
    TEST_ASSERT_EQUAL_INT32(-1, hx711_parse(raw));
}

/* raw {0x80,0x00,0x00} -> -8388608 (0x800000 sign-extended, most negative). */
static void test_hx711_parse_min_negative_returns_neg_8388608(void)
{
    const uint8_t raw[3] = { 0x80, 0x00, 0x00 };
    TEST_ASSERT_EQUAL_INT32(-8388608, hx711_parse(raw));
}

/* NULL -> 0. */
static void test_hx711_parse_null_returns_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(0, hx711_parse(NULL));
}

/* Sign-extension correctness: a mid-range negative value round-trips exactly.
 * 0xFFFFFE = -2 (sign-extended). Guards against off-by-one / mask errors in the
 * uint32 OR-mask path (no signed shift). The UBSan+ASan cleanliness of this path
 * is enforced by the `make -C esp-libs sanitize` gate over the whole suite. */
static void test_hx711_parse_sign_extends_via_mask_no_signed_shift(void)
{
    const uint8_t raw[3] = { 0xFF, 0xFF, 0xFE };
    TEST_ASSERT_EQUAL_INT32(-2, hx711_parse(raw));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hx711_parse_zero_returns_zero);
    RUN_TEST(test_hx711_parse_one_returns_one);
    RUN_TEST(test_hx711_parse_max_positive_returns_8388607);
    RUN_TEST(test_hx711_parse_all_ones_returns_minus_one);
    RUN_TEST(test_hx711_parse_min_negative_returns_neg_8388608);
    RUN_TEST(test_hx711_parse_null_returns_zero);
    RUN_TEST(test_hx711_parse_sign_extends_via_mask_no_signed_shift);
    return UNITY_END();
}
