/* Unity host test for the pure part of the BH1750 L3 driver.
 * Tests ONLY the pure API contract in bh1750.h (bh1750_parse); the SDK glue
 * (bh1750_dev.{h,c} over i2c_bus) is COMPILE_GATE-only and not exercised here.
 * The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "bh1750.h"

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance 1: raw {0x00,0x06} -> val 6 -> *milli_lux == 5000 (6*2500/3). */
static void test_bh1750_parse_val_6_is_5000_milli_lux(void)
{
    const uint8_t raw[2] = { 0x00, 0x06 };
    uint32_t milli_lux = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &milli_lux));
    TEST_ASSERT_EQUAL_UINT32(5000u, milli_lux);
}

/* Acceptance 2: raw {0x00,0x0C} -> val 12 -> *milli_lux == 10000 (12*2500/3). */
static void test_bh1750_parse_val_12_is_10000_milli_lux(void)
{
    const uint8_t raw[2] = { 0x00, 0x0C };
    uint32_t milli_lux = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &milli_lux));
    TEST_ASSERT_EQUAL_UINT32(10000u, milli_lux);
}

/* Acceptance 3: raw {0xFF,0xFF} -> val 65535 -> *milli_lux == 54612500
 * (65535*2500/3). Full-scale: the high byte is shifted on a non-negative
 * uint32, so no overflow/UB and the truncating divide matches the spec. */
static void test_bh1750_parse_full_scale_is_54612500_milli_lux(void)
{
    const uint8_t raw[2] = { 0xFF, 0xFF };
    uint32_t milli_lux = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &milli_lux));
    TEST_ASSERT_EQUAL_UINT32(54612500u, milli_lux);
}

/* Acceptance 4: raw==NULL or milli_lux==NULL -> false and writes no output.
 * The output cell is pre-seeded with a sentinel and must remain untouched. */
static void test_bh1750_parse_null_guard_returns_false_and_writes_nothing(void)
{
    const uint8_t raw[2] = { 0x00, 0x06 };
    uint32_t milli_lux = 0xDEADBEEFu;

    /* raw == NULL */
    TEST_ASSERT_FALSE(bh1750_parse(NULL, &milli_lux));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, milli_lux);

    /* milli_lux == NULL (no crash, no write — just a defined false) */
    TEST_ASSERT_FALSE(bh1750_parse(raw, NULL));

    /* both NULL */
    TEST_ASSERT_FALSE(bh1750_parse(NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFu, milli_lux);
}

/* Acceptance 5: milli_lux = val*2500/3 with val=(raw[0]<<8)|raw[1]; the shift
 * is on a non-negative uint32 only (no UB under UBSan). Exercise a vector whose
 * high byte is set so the <<8 is meaningfully wide: raw {0x12,0x34} -> val 4660
 * -> 4660*2500/3 = 11650000/3 = 3883333 (integer-truncated). */
static void test_bh1750_parse_high_byte_shift_no_ub(void)
{
    const uint8_t raw[2] = { 0x12, 0x34 };
    uint32_t milli_lux = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &milli_lux));
    TEST_ASSERT_EQUAL_UINT32(3883333u, milli_lux);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bh1750_parse_val_6_is_5000_milli_lux);
    RUN_TEST(test_bh1750_parse_val_12_is_10000_milli_lux);
    RUN_TEST(test_bh1750_parse_full_scale_is_54612500_milli_lux);
    RUN_TEST(test_bh1750_parse_null_guard_returns_false_and_writes_nothing);
    RUN_TEST(test_bh1750_parse_high_byte_shift_no_ub);
    return UNITY_END();
}
