/* Unity host test for the pure part of the BME280 L3 driver.
 * Tests ONLY the API contract in bme280_parse.h (parse_calib + compensate);
 * the SDK glue (bme280.{h,c} over i2c_bus) is compile-gate-only, not exercised here.
 * The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "bme280_parse.h"

void setUp(void)    {}
void tearDown(void) {}

/* Primary calib register blocks (design spec cross-checked vector):
 *   calib26 = 0x88..0xA1 (26 bytes), calibH = 0xE1..0xE7 (7 bytes). */
static const uint8_t CALIB26[26] = {
    0x45, 0x6F, 0x6F, 0x68, 0x32, 0x00, 0xF4, 0x93, 0x43, 0xD6, 0xD0, 0x0B, 0xBC,
    0x18, 0xEB, 0xFF, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17, 0x00, 0x4B
};
static const uint8_t CALIBH[7] = { 0x72, 0x01, 0x00, 0x12, 0x02, 0x00, 0x1E };

/* Acceptance 1: parse_calib turns the register blocks into the documented
 * trimming coefficients (all fields). */
static void test_bme280_parse_calib_all_fields(void)
{
    bme280_calib cal = {0};
    TEST_ASSERT_TRUE(bme280_parse_calib(CALIB26, CALIBH, &cal));

    TEST_ASSERT_EQUAL_UINT16(28485, cal.T1);
    TEST_ASSERT_EQUAL_INT16(26735,  cal.T2);
    TEST_ASSERT_EQUAL_INT16(50,     cal.T3);

    TEST_ASSERT_EQUAL_UINT16(37876, cal.P1);
    TEST_ASSERT_EQUAL_INT16(-10685, cal.P2);
    TEST_ASSERT_EQUAL_INT16(3024,   cal.P3);
    TEST_ASSERT_EQUAL_INT16(6332,   cal.P4);
    TEST_ASSERT_EQUAL_INT16(-21,    cal.P5);
    TEST_ASSERT_EQUAL_INT16(-7,     cal.P6);
    TEST_ASSERT_EQUAL_INT16(15500,  cal.P7);
    TEST_ASSERT_EQUAL_INT16(-14600, cal.P8);
    TEST_ASSERT_EQUAL_INT16(6000,   cal.P9);

    TEST_ASSERT_EQUAL_UINT8(75,  cal.H1);
    TEST_ASSERT_EQUAL_INT16(370, cal.H2);
    TEST_ASSERT_EQUAL_UINT8(0,   cal.H3);
    TEST_ASSERT_EQUAL_INT16(290, cal.H4);
    TEST_ASSERT_EQUAL_INT16(0,   cal.H5);
    TEST_ASSERT_EQUAL_INT8(30,   cal.H6);
}

/* Acceptance 2: the packed 12-bit signed H4/H5 are sign-extended 12->16 bits.
 * calibH[3..5] = {0xFC, 0xCE, 0xF9} encode H4 == -50, H5 == -100 — a case the
 * primary vector's positive H4/H5 does not exercise. */
static void test_bme280_parse_calib_h4_h5_sign_extension(void)
{
    uint8_t calibH[7] = { 0x72, 0x01, 0x00, 0xFC, 0xCE, 0xF9, 0x1E };
    bme280_calib cal = {0};
    TEST_ASSERT_TRUE(bme280_parse_calib(CALIB26, calibH, &cal));

    TEST_ASSERT_EQUAL_INT16(-50,  cal.H4);
    TEST_ASSERT_EQUAL_INT16(-100, cal.H5);
}

/* Acceptance 3: Bosch integer compensation, converted to the SI output vector. */
static void test_bme280_compensate_si_vector(void)
{
    bme280_calib cal = {0};
    TEST_ASSERT_TRUE(bme280_parse_calib(CALIB26, CALIBH, &cal));

    bme280_reading r = {0};
    TEST_ASSERT_TRUE(bme280_compensate(&cal, 519888, 326816, 26083, &r));

    TEST_ASSERT_EQUAL_INT32(20440,   r.temp_mc);   /* 20.44 C */
    TEST_ASSERT_EQUAL_UINT32(101576,  r.press_pa);  /* 1015.76 hPa */
    TEST_ASSERT_EQUAL_UINT32(42743,   r.humi_mrh);  /* 42.743 %RH */
}

/* Acceptance 4a: parse_calib NULL-guards every pointer argument. */
static void test_bme280_parse_calib_null_guards(void)
{
    bme280_calib cal = {0};
    TEST_ASSERT_FALSE(bme280_parse_calib(NULL,    CALIBH, &cal));
    TEST_ASSERT_FALSE(bme280_parse_calib(CALIB26, NULL,   &cal));
    TEST_ASSERT_FALSE(bme280_parse_calib(CALIB26, CALIBH, NULL));
}

/* Acceptance 4b: compensate NULL-guards calib and out. */
static void test_bme280_compensate_null_guards(void)
{
    bme280_calib cal = {0};
    bme280_reading r = {0};
    TEST_ASSERT_FALSE(bme280_compensate(NULL, 519888, 326816, 26083, &r));
    TEST_ASSERT_FALSE(bme280_compensate(&cal, 519888, 326816, 26083, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bme280_parse_calib_all_fields);
    RUN_TEST(test_bme280_parse_calib_h4_h5_sign_extension);
    RUN_TEST(test_bme280_compensate_si_vector);
    RUN_TEST(test_bme280_parse_calib_null_guards);
    RUN_TEST(test_bme280_compensate_null_guards);
    return UNITY_END();
}
