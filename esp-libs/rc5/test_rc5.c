/* Unity host test for the pure part of the RC5 IR L3 driver.
 * Tests ONLY the API contract in rc5.h (rc5_decode); the SDK glue
 * (rc5_rx.{h,c} doing GPIO line sampling) is compile-gate-only and is NOT
 * exercised here. The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "rc5.h"

void setUp(void)    {}
void tearDown(void) {}

/* RC5 carries a 14-bit frame (S1 S2 T A4..A0 C5..C0), transmitted MSB-first as
 * 28 Manchester half-bits: a logical 1 is the pair (0,1), a logical 0 is (1,0).
 * Build the 28 half-bit samples from a 14-bit raw value, MSB (bit13) first. */
#define RC5_BITS       14
#define RC5_HALFBITS   28

static void build_rc5(uint8_t hb[RC5_HALFBITS], uint16_t raw)
{
    size_t i = 0;
    for (int bit = RC5_BITS - 1; bit >= 0; bit--) {   /* MSB-first */
        uint8_t b = (uint8_t)((raw >> bit) & 1u);
        if (b) {            /* logical 1 -> (0,1) */
            hb[i++] = 0;
            hb[i++] = 1;
        } else {            /* logical 0 -> (1,0) */
            hb[i++] = 1;
            hb[i++] = 0;
        }
    }
    /* i == RC5_HALFBITS here. */
}

/* Acceptance 1: 28 half-bits built MSB-first from raw 0x3535 decode true with
 * address 0x14, command 0x35, toggle 0, raw 0x3535.
 *   0x3535 = 11 0101 0011 0101b: S1=1 (start ok), toggle=bit11=0,
 *   address=(raw>>6)&0x1F = 0x14, command=raw&0x3F = 0x35. */
static void test_rc5_decode_valid_0x3535(void)
{
    uint8_t hb[RC5_HALFBITS];
    build_rc5(hb, 0x3535);

    rc5_result r = {0};
    TEST_ASSERT_TRUE(rc5_decode(hb, RC5_HALFBITS, &r));
    TEST_ASSERT_EQUAL_UINT8(0x14, r.address);
    TEST_ASSERT_EQUAL_UINT8(0x35, r.command);
    TEST_ASSERT_EQUAL_UINT8(0,    r.toggle);
    TEST_ASSERT_EQUAL_HEX16(0x3535, r.raw);
}

/* Acceptance 2: a valid 0x3535 frame whose first half-bit pair is corrupted to
 * (1,1) is not a valid Manchester pair and must be rejected. */
static void test_rc5_decode_bad_manchester_pair_fails(void)
{
    uint8_t hb[RC5_HALFBITS];
    build_rc5(hb, 0x3535);
    hb[0] = 1;   /* first pair (0,1) -> (1,1): invalid */
    hb[1] = 1;

    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, RC5_HALFBITS, &r));
}

/* Acceptance 3: a frame whose leading start bit S1 is 0 (raw 0x1535, bit13==0)
 * fails start-bit validation. */
static void test_rc5_decode_start_bit_zero_fails(void)
{
    uint8_t hb[RC5_HALFBITS];
    build_rc5(hb, 0x1535);   /* (raw>>13)&1 == 0 */

    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, RC5_HALFBITS, &r));
}

/* Acceptance 4: a half-bit count other than 28 (e.g. 10) is rejected. */
static void test_rc5_decode_wrong_count_fails(void)
{
    uint8_t hb[RC5_HALFBITS];
    build_rc5(hb, 0x3535);

    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, 10, &r));
}

/* Acceptance 5: NULL halfbits or NULL out are guarded and return false. */
static void test_rc5_decode_null_guards(void)
{
    uint8_t hb[RC5_HALFBITS];
    build_rc5(hb, 0x3535);

    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(NULL, RC5_HALFBITS, &r));
    TEST_ASSERT_FALSE(rc5_decode(hb,   RC5_HALFBITS, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rc5_decode_valid_0x3535);
    RUN_TEST(test_rc5_decode_bad_manchester_pair_fails);
    RUN_TEST(test_rc5_decode_start_bit_zero_fails);
    RUN_TEST(test_rc5_decode_wrong_count_fails);
    RUN_TEST(test_rc5_decode_null_guards);
    return UNITY_END();
}
