/* Unity host test for the pure part of the DS3231 L3 driver.
 * Tests ONLY the API contract in ds3231.h (BCD decode/encode + on-chip temp);
 * the SDK glue (ds3231_dev.{h,c} over i2c_bus) is compile-gate-only / deferred,
 * not exercised here. The header does not exist yet — this is the RED step. */
#include "unity.h"
#include "ds3231.h"

void setUp(void)    {}
void tearDown(void) {}

/* Cross-checked register vector: 24-hour BCD time
 *   sec=30 min=45 hour=13 dow=6 date=27 month=6 year=2026. */
static const uint8_t RAW7[7] = {0x30, 0x45, 0x13, 0x06, 0x27, 0x06, 0x26};

/* Acceptance 1: ds3231_decode turns the 7 BCD registers into the documented
 * fields (24-hour mode), returning true. */
static void test_ds3231_decode_vector_all_fields(void)
{
    ds3231_time t = {0};
    TEST_ASSERT_TRUE(ds3231_decode(RAW7, &t));

    TEST_ASSERT_EQUAL_UINT8(30,  t.sec);
    TEST_ASSERT_EQUAL_UINT8(45,  t.min);
    TEST_ASSERT_EQUAL_UINT8(13,  t.hour);
    TEST_ASSERT_EQUAL_UINT8(6,   t.dow);
    TEST_ASSERT_EQUAL_UINT8(27,  t.date);
    TEST_ASSERT_EQUAL_UINT8(6,   t.month);
    TEST_ASSERT_EQUAL_UINT16(2026, t.year);
}

/* Acceptance 2: ds3231_encode is the exact inverse of ds3231_decode — the 7
 * bytes produced from the decoded struct equal the original input (round-trip). */
static void test_ds3231_encode_round_trip(void)
{
    ds3231_time t = {0};
    TEST_ASSERT_TRUE(ds3231_decode(RAW7, &t));

    uint8_t raw[7] = {0};
    TEST_ASSERT_TRUE(ds3231_encode(&t, raw));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(RAW7, raw, 7);
}

/* Acceptance 3: positive on-chip temperature.
 *   msb=0x19 (25), lsb=0x40 (lsb>>6 == 1 -> 250 mC) -> 25250 mC (25.25 degC). */
static void test_ds3231_temp_mc_positive(void)
{
    TEST_ASSERT_EQUAL_INT32(25250, ds3231_temp_mc(0x19, 0x40));
}

/* Acceptance 4: negative on-chip temperature with the sign in the MSB.
 *   msb=0xFF -> (int8_t) == -1 -> -1000 mC; lsb=0x80 (lsb>>6 == 2 -> 500 mC)
 *   -> -500 mC (-0.5 degC). Exercises the signed (int8_t) cast, NOT a signed
 *   left-shift of a negative value. */
static void test_ds3231_temp_mc_negative_signed_msb(void)
{
    TEST_ASSERT_EQUAL_INT32(-500, ds3231_temp_mc(0xFF, 0x80));
}

/* Acceptance 5: NULL-guards on both pure functions.
 *   decode(NULL,out) / decode(raw,NULL) and encode(NULL,raw) / encode(t,NULL)
 *   all return false. */
static void test_ds3231_null_guards(void)
{
    ds3231_time t = {0};
    uint8_t raw[7] = {0};

    TEST_ASSERT_FALSE(ds3231_decode(NULL, &t));
    TEST_ASSERT_FALSE(ds3231_decode(RAW7, NULL));
    TEST_ASSERT_FALSE(ds3231_encode(NULL, raw));
    TEST_ASSERT_FALSE(ds3231_encode(&t, NULL));
}

/* Acceptance 6: the conversion stays clean under UBSan — temp sign via (int8_t)
 * cast + integer multiply, no left-shift of a possibly-negative value. The most
 * negative MSB (0x80 == -128) and full lsb fraction exercise the signed path
 * that a `<< n` would make UB:  -128*1000 + 3*250 = -127250 mC. */
static void test_ds3231_temp_mc_most_negative_clean_ub(void)
{
    TEST_ASSERT_EQUAL_INT32(-127250, ds3231_temp_mc(0x80, 0xC0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ds3231_decode_vector_all_fields);
    RUN_TEST(test_ds3231_encode_round_trip);
    RUN_TEST(test_ds3231_temp_mc_positive);
    RUN_TEST(test_ds3231_temp_mc_negative_signed_msb);
    RUN_TEST(test_ds3231_null_guards);
    RUN_TEST(test_ds3231_temp_mc_most_negative_clean_ub);
    return UNITY_END();
}
