/* Unity host test for the PURE part of the ADS1115 L3 ADC driver.
 * Tests ONLY the API contract in ads1115.h (ads1115_to_microvolts +
 * ads1115_config); the SDK glue (ads1115_dev.{h,c} over i2c_bus) is
 * COMPILE_GATE-only / deferred and is NOT exercised here.
 * The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "ads1115.h"

void setUp(void)    {}
void tearDown(void) {}

/* --- ads1115_to_microvolts: raw count * FSR_uV[pga] / 32768 (int64 widen
 *     BEFORE multiply); FSR_uV = {6144000,4096000,2048000,1024000,512000,256000}.
 * Each acceptance value cross-checks the truncating-toward-zero integer math. */

/* Acceptance 1: full positive scale at PGA 4.096V.
 *   32767 * 4096000 / 32768 = 4095875 (truncated). */
static void test_ads1115_to_microvolts_pos_fullscale_4v096(void)
{
    TEST_ASSERT_EQUAL_INT32(4095875, ads1115_to_microvolts(32767, ADS1115_PGA_4V096));
}

/* Acceptance 2: full negative scale at PGA 4.096V.
 *   -32768 * 4096000 / 32768 = -4096000 exactly. The int64 widen BEFORE the
 *   multiply is what keeps -32768 * 4096000 from overflowing 32-bit. */
static void test_ads1115_to_microvolts_neg_fullscale_4v096(void)
{
    TEST_ASSERT_EQUAL_INT32(-4096000, ads1115_to_microvolts(-32768, ADS1115_PGA_4V096));
}

/* Acceptance 3: half scale at PGA 4.096V.
 *   16384 * 4096000 / 32768 = 2048000 exactly. */
static void test_ads1115_to_microvolts_half_scale_4v096(void)
{
    TEST_ASSERT_EQUAL_INT32(2048000, ads1115_to_microvolts(16384, ADS1115_PGA_4V096));
}

/* Acceptance 4: zero raw -> 0 microvolts for any PGA. */
static void test_ads1115_to_microvolts_zero_any_pga(void)
{
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_6V144));
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_4V096));
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_2V048));
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_1V024));
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_0V512));
    TEST_ASSERT_EQUAL_INT32(0, ads1115_to_microvolts(0, ADS1115_PGA_0V256));
}

/* Acceptance 5: full positive scale at PGA 6.144V.
 *   32767 * 6144000 / 32768 = 6143812 (truncated). */
static void test_ads1115_to_microvolts_pos_fullscale_6v144(void)
{
    TEST_ASSERT_EQUAL_INT32(6143812, ads1115_to_microvolts(32767, ADS1115_PGA_6V144));
}

/* Acceptance 6: full positive scale at PGA 0.256V.
 *   32767 * 256000 / 32768 = 255992 (truncated). */
static void test_ads1115_to_microvolts_pos_fullscale_0v256(void)
{
    TEST_ASSERT_EQUAL_INT32(255992, ads1115_to_microvolts(32767, ADS1115_PGA_0V256));
}

/* Contract guard: a PGA value outside the enum clamps to 2.048V (index 2)
 * rather than indexing the FSR table out of bounds. Exercises the documented
 * clamp at full positive scale: 32767 * 2048000 / 32768 = 2047937. */
static void test_ads1115_to_microvolts_pga_out_of_range_clamps_2v048(void)
{
    int32_t clamped  = ads1115_to_microvolts(32767, (ads1115_pga)99);
    int32_t expected = ads1115_to_microvolts(32767, ADS1115_PGA_2V048);
    TEST_ASSERT_EQUAL_INT32(2047937, expected);
    TEST_ASSERT_EQUAL_INT32(expected, clamped);
}

/* --- ads1115_config: 0x8000 | ((0x4|(channel&3))<<12) | (pga<<9)
 *     | 0x0100 | 0x0080 | 0x0003. */

/* Acceptance 7: channel 0, PGA 2.048V -> 0xC583. */
static void test_ads1115_config_ch0_2v048(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xC583, ads1115_config(0, ADS1115_PGA_2V048));
}

/* Acceptance 8: channel 3, PGA 4.096V -> 0xF383. */
static void test_ads1115_config_ch3_4v096(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xF383, ads1115_config(3, ADS1115_PGA_4V096));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ads1115_to_microvolts_pos_fullscale_4v096);
    RUN_TEST(test_ads1115_to_microvolts_neg_fullscale_4v096);
    RUN_TEST(test_ads1115_to_microvolts_half_scale_4v096);
    RUN_TEST(test_ads1115_to_microvolts_zero_any_pga);
    RUN_TEST(test_ads1115_to_microvolts_pos_fullscale_6v144);
    RUN_TEST(test_ads1115_to_microvolts_pos_fullscale_0v256);
    RUN_TEST(test_ads1115_to_microvolts_pga_out_of_range_clamps_2v048);
    RUN_TEST(test_ads1115_config_ch0_2v048);
    RUN_TEST(test_ads1115_config_ch3_4v096);
    return UNITY_END();
}
