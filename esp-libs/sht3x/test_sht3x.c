/* Unity host test for the pure part of the SHT3x L3 driver.
 * Tests ONLY the API contract in sht3x.h (sht3x_parse over a 6-byte frame);
 * the SDK glue (sht3x_dev.{h,c} over i2c_bus) is compile-gate-only and DEFERRED,
 * not exercised here. The header does not exist yet — this is the RED step of TDD.
 *
 * sht3x_parse contract:
 *   bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh);
 *   6-byte frame [t_msb,t_lsb,t_crc, h_msb,h_lsb,h_crc].
 *   Validate Sensirion CRC8 (poly 0x31, init 0xFF, MSB-first, no final xor) over
 *   raw[0..1] vs raw[2] and raw[3..4] vs raw[5]; any mismatch -> false.
 *   raw_t = (t_msb<<8)|t_lsb, raw_h likewise.
 *   temp_mc  = -45000 + (int32_t)((int64_t)175000*raw_t/65535);
 *   humi_mrh =          (int32_t)((int64_t)100000*raw_h/65535);
 *   NULL (any of raw/temp_mc/humi_mrh) -> false. */
#include "unity.h"
#include "sht3x.h"

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance 1: Sensirion CRC8 anchor — CRC8({0x00,0x00})==0x81 and
 * CRC8({0xBE,0xEF})==0x92. CRC8 is internal to sht3x_parse (not a public fn),
 * so the anchor is exercised through the public API: frames carrying exactly
 * these CRC bytes must validate (parse returns true). A wrong poly/init would
 * compute a different byte and reject these frames. */
static void test_sht3x_crc8_sensirion_anchors_accept(void)
{
    /* 0x81 is the Sensirion CRC8 of {0x00,0x00}; 0x92 of {0xBE,0xEF}. */
    uint8_t frame_00[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x81};
    uint8_t frame_be[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x92};
    int32_t t = 0, h = 0;

    TEST_ASSERT_TRUE(sht3x_parse(frame_00, &t, &h));
    TEST_ASSERT_TRUE(sht3x_parse(frame_be, &t, &h));
}

/* Acceptance 2: {0x00,0x00,0x81, 0x00,0x00,0x81} -> true,
 * *temp_mc == -45000 and *humi_mrh == 0. */
static void test_sht3x_parse_zero_frame(void)
{
    uint8_t raw[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x81};
    int32_t t = 12345, h = 12345;

    TEST_ASSERT_TRUE(sht3x_parse(raw, &t, &h));
    TEST_ASSERT_EQUAL_INT32(-45000, t);
    TEST_ASSERT_EQUAL_INT32(0,      h);
}

/* Acceptance 3: {0xBE,0xEF,0x92, 0xBE,0xEF,0x92} (raw 0xBEEF == 48879) -> true,
 * *temp_mc == 85523 and *humi_mrh == 74584. Cross-checked against the int64
 * widen-before-multiply conversion. */
static void test_sht3x_parse_beef_frame(void)
{
    uint8_t raw[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x92};
    int32_t t = 0, h = 0;

    TEST_ASSERT_TRUE(sht3x_parse(raw, &t, &h));
    TEST_ASSERT_EQUAL_INT32(85523, t);
    TEST_ASSERT_EQUAL_INT32(74584, h);
}

/* Acceptance 4: {0x00,0x00,0x00, 0x00,0x00,0x81} — first (temperature) CRC is
 * wrong (0x00 instead of 0x81) -> false. */
static void test_sht3x_parse_bad_first_crc(void)
{
    uint8_t raw[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x81};
    int32_t t = 0, h = 0;

    TEST_ASSERT_FALSE(sht3x_parse(raw, &t, &h));
}

/* Acceptance 4 (companion): a wrong humidity CRC is also rejected — guards
 * against checking only the first block. */
static void test_sht3x_parse_bad_second_crc(void)
{
    uint8_t raw[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x00};
    int32_t t = 0, h = 0;

    TEST_ASSERT_FALSE(sht3x_parse(raw, &t, &h));
}

/* Acceptance 5: raw==NULL or temp_mc==NULL or humi_mrh==NULL -> false. */
static void test_sht3x_parse_null_guards(void)
{
    uint8_t raw[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x81};
    int32_t t = 0, h = 0;

    TEST_ASSERT_FALSE(sht3x_parse(NULL, &t,   &h));
    TEST_ASSERT_FALSE(sht3x_parse(raw,  NULL, &h));
    TEST_ASSERT_FALSE(sht3x_parse(raw,  &t,   NULL));
}

/* Acceptance 6: conversion widens to int64 before multiply (no overflow) and the
 * CRC8 loop runs on non-negative uint8 (no signed/negative left-shift) — clean
 * under UBSan. raw==0xFFFF is the conversion ceiling (175000*65535 overflows a
 * 32-bit int if not widened) and 0xFF bytes exercise the CRC top-bit shift path. */
static void test_sht3x_parse_ffff_no_overflow(void)
{
    /* Sensirion CRC8 of {0xFF,0xFF} == 0xAC. raw 0xFFFF == 65535:
     *   temp_mc  = -45000 + 175000*65535/65535 = 130000
     *   humi_mrh =          100000*65535/65535 = 100000 */
    uint8_t raw[6] = {0xFF, 0xFF, 0xAC, 0xFF, 0xFF, 0xAC};
    int32_t t = 0, h = 0;

    TEST_ASSERT_TRUE(sht3x_parse(raw, &t, &h));
    TEST_ASSERT_EQUAL_INT32(130000, t);
    TEST_ASSERT_EQUAL_INT32(100000, h);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sht3x_crc8_sensirion_anchors_accept);
    RUN_TEST(test_sht3x_parse_zero_frame);
    RUN_TEST(test_sht3x_parse_beef_frame);
    RUN_TEST(test_sht3x_parse_bad_first_crc);
    RUN_TEST(test_sht3x_parse_bad_second_crc);
    RUN_TEST(test_sht3x_parse_null_guards);
    RUN_TEST(test_sht3x_parse_ffff_no_overflow);
    return UNITY_END();
}
