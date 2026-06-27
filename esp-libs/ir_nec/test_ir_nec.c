/* Unity host test for the pure part of the IR-NEC L3 driver.
 * Tests ONLY the API contract in ir_nec.h (ir_nec_decode); the SDK glue
 * (ir_nec_rx.{h,c} doing GPIO edge capture) is compile-gate-only and is NOT
 * exercised here. The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "ir_nec.h"

void setUp(void)    {}
void tearDown(void) {}

/* NEC mark/space durations (microseconds), nominal:
 *   leading mark 9000, leading space 4500, per data bit mark 562,
 *   data space 562 -> logical 0, 1687 -> logical 1 (1124us midpoint).
 * A full frame is 66 entries: 2 header + 32*(mark,space). */
#define IR_LEAD_MARK   9000
#define IR_LEAD_SPACE  4500
#define IR_BIT_MARK     562
#define IR_SPACE_0      562
#define IR_SPACE_1     1687
#define IR_FRAME_LEN     66

/* Build a 66-entry mark/space array from the 4 NEC bytes, in wire order
 * addr, ~addr, cmd, ~cmd; each byte LSB-first, mark then space per bit. */
static void build_frame(uint16_t us[IR_FRAME_LEN],
                        uint8_t addr, uint8_t naddr,
                        uint8_t cmd,  uint8_t ncmd)
{
    const uint8_t bytes[4] = { addr, naddr, cmd, ncmd };
    size_t i = 0;
    us[i++] = IR_LEAD_MARK;
    us[i++] = IR_LEAD_SPACE;
    for (size_t b = 0; b < 4; b++) {
        for (size_t bit = 0; bit < 8; bit++) {   /* LSB-first */
            us[i++] = IR_BIT_MARK;
            us[i++] = (bytes[b] & (1u << bit)) ? IR_SPACE_1 : IR_SPACE_0;
        }
    }
    /* i == IR_FRAME_LEN here. */
}

/* Acceptance 1: a valid frame for address 0x00, command 0x16
 * (bytes addr=0x00, ~addr=0xFF, cmd=0x16, ~cmd=0xE9; raw==0xE916FF00)
 * decodes true and fills address/command/raw. */
static void test_ir_nec_decode_valid_addr00_cmd16(void)
{
    uint16_t us[IR_FRAME_LEN];
    build_frame(us, 0x00, 0xFF, 0x16, 0xE9);

    ir_nec_result r = {0};
    TEST_ASSERT_TRUE(ir_nec_decode(us, IR_FRAME_LEN, &r));
    TEST_ASSERT_EQUAL_UINT8(0x00, r.address);
    TEST_ASSERT_EQUAL_UINT8(0x16, r.command);
    TEST_ASSERT_EQUAL_HEX32(0xE916FF00u, r.raw);
}

/* Acceptance 2: a frame whose command does not match its ~command fails the
 * NEC complement check. Flip cmd bit0: cmd byte becomes 0x17 while ~cmd byte
 * stays 0xE9, so (raw>>24)&0xFF (0xE9) != (~0x17)&0xFF (0xE8) -> false. */
static void test_ir_nec_decode_flipped_cmd_bit_fails_complement(void)
{
    uint16_t us[IR_FRAME_LEN];
    build_frame(us, 0x00, 0xFF, 0x17, 0xE9);   /* 0x17 vs ~cmd byte 0xE9 */

    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, IR_FRAME_LEN, &r));
}

/* Acceptance 3: a bad leading mark (3000us, far outside the +/-25% header
 * tolerance 6750..11250) is rejected. */
static void test_ir_nec_decode_bad_header_fails(void)
{
    uint16_t us[IR_FRAME_LEN];
    build_frame(us, 0x00, 0xFF, 0x16, 0xE9);
    us[0] = 3000;   /* leading mark out of range */

    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, IR_FRAME_LEN, &r));
}

/* Acceptance 4: a count below the required 66 entries is rejected. */
static void test_ir_nec_decode_short_count_fails(void)
{
    uint16_t us[IR_FRAME_LEN];
    build_frame(us, 0x00, 0xFF, 0x16, 0xE9);

    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, 10, &r));
}

/* Acceptance 5: NULL us or NULL out are guarded and return false. */
static void test_ir_nec_decode_null_guards(void)
{
    uint16_t us[IR_FRAME_LEN];
    build_frame(us, 0x00, 0xFF, 0x16, 0xE9);

    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(NULL, IR_FRAME_LEN, &r));
    TEST_ASSERT_FALSE(ir_nec_decode(us,   IR_FRAME_LEN, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ir_nec_decode_valid_addr00_cmd16);
    RUN_TEST(test_ir_nec_decode_flipped_cmd_bit_fails_complement);
    RUN_TEST(test_ir_nec_decode_bad_header_fails);
    RUN_TEST(test_ir_nec_decode_short_count_fails);
    RUN_TEST(test_ir_nec_decode_null_guards);
    return UNITY_END();
}
