/* Unity host test for `rng` — xorshift32 PRNG object.
 *
 * RED step (TDD): this test is written against the FROZEN API contract only
 * (see docs/superpowers/specs/2026-06-27-clibs-common-oop-batch2-design.md,
 * "Module specs -> rng"). No implementation (rng.c) and no header (rng.h)
 * exist yet, so this MUST fail to build/link until the Dev provides them.
 *
 * Contract under test (header does not exist yet — declared by the contract):
 *   typedef struct { uint32_t state; } rng;        // fields private
 *   void     rng_seed (rng *self, uint32_t seed);  // seed 0 coerced to 1
 *   uint32_t rng_next (rng *self);                 // xorshift32; 0 if NULL self
 *   uint32_t rng_below(rng *self, uint32_t bound); // [0,bound); bound 0 -> 0
 */
#include "rng.h"
#include "unity.h"

#include <stdint.h>
#include <stdbool.h>

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance #1:
 * Two rng seeded with the same value produce the identical rng_next sequence
 * (reproducible / deterministic from a seed). */
void test_rng_same_seed_produces_identical_sequence(void)
{
    rng a;
    rng b;
    rng_seed(&a, 0xC0FFEEu);
    rng_seed(&b, 0xC0FFEEu);

    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_UINT32(rng_next(&a), rng_next(&b));
    }
}

/* Acceptance #2:
 * A single seeded rng's successive rng_next values are NOT all equal
 * (i.e. the generator actually advances / produces variation). */
void test_rng_successive_values_not_all_equal(void)
{
    rng r;
    rng_seed(&r, 12345u);

    uint32_t first = rng_next(&r);
    bool saw_different = false;
    for (int i = 0; i < 32; i++) {
        if (rng_next(&r) != first) {
            saw_different = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(saw_different,
        "successive rng_next values were all identical");
}

/* Acceptance #3:
 * rng_seed(self, 0) coerces internal state to 1 (xorshift cannot start at 0),
 * so rng_next is non-zero and remains deterministic across two such rngs. */
void test_rng_seed_zero_is_nonzero_and_deterministic(void)
{
    rng r;
    rng_seed(&r, 0u);
    uint32_t v = rng_next(&r);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, v);

    /* Determinism: another rng seeded with 0 yields the same sequence. */
    rng r2;
    rng_seed(&r2, 0u);
    TEST_ASSERT_EQUAL_UINT32(v, rng_next(&r2));
    TEST_ASSERT_EQUAL_UINT32(rng_next(&r), rng_next(&r2));
}

/* Acceptance #4:
 * rng_below(self, 10) over 1000 draws is always in [0, 10). */
void test_rng_below_stays_within_bound(void)
{
    rng r;
    rng_seed(&r, 987654321u);

    for (int i = 0; i < 1000; i++) {
        uint32_t v = rng_below(&r, 10u);
        TEST_ASSERT_TRUE_MESSAGE(v < 10u, "rng_below(.,10) returned a value >= 10");
    }
}

/* Acceptance #5:
 * rng_below(self, 1) == 0 (only value in [0,1)); rng_below(self, 0) == 0
 * (bound 0 is defined to return 0). */
void test_rng_below_one_and_zero_return_zero(void)
{
    rng r;
    rng_seed(&r, 42u);

    TEST_ASSERT_EQUAL_UINT32(0u, rng_below(&r, 1u));
    TEST_ASSERT_EQUAL_UINT32(0u, rng_below(&r, 0u));
}

/* Acceptance #6:
 * NULL self: rng_next(NULL)==0, rng_below(NULL,5)==0, rng_seed(NULL,1) no crash. */
void test_rng_null_self_is_safe(void)
{
    rng_seed(NULL, 1u); /* must not crash (no-op) */

    TEST_ASSERT_EQUAL_UINT32(0u, rng_next(NULL));
    TEST_ASSERT_EQUAL_UINT32(0u, rng_below(NULL, 5u));
}

/* Acceptance #7:
 * No UB: the xorshift (x^=x<<13; x^=x>>17; x^=x<<5) and modulo operate only on
 * uint32_t (no left-shift of a negative value). Observable, host-checkable
 * proxies for "unsigned shifts happen and produce a full-width 32-bit result":
 *   - the generator is deterministic & reproducible (already implied), and
 *   - across a run it yields a value with the high bit (bit 31) set, which a
 *     correct uint32_t left-shift produces but signed-shift UB / accidental
 *     sign-extension would not reliably produce. (Run under UBSan per DoD to
 *     confirm cleanliness directly.) */
void test_rng_shifts_operate_on_uint32_no_signed_ub(void)
{
    rng r;
    rng_seed(&r, 0xABCDEF01u);

    bool saw_high_bit = false;
    for (int i = 0; i < 256; i++) {
        uint32_t v = rng_next(&r);
        if (v & 0x80000000u) {
            saw_high_bit = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(saw_high_bit,
        "expected at least one rng_next value with bit 31 set (uint32_t shifts)");

    /* Reproducibility cross-check: the exact same seed replays the exact same
     * first value — the xorshift step is a pure function of uint32_t state. */
    rng r2;
    rng_seed(&r2, 0xABCDEF01u);
    rng r3;
    rng_seed(&r3, 0xABCDEF01u);
    TEST_ASSERT_EQUAL_UINT32(rng_next(&r2), rng_next(&r3));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rng_same_seed_produces_identical_sequence);
    RUN_TEST(test_rng_successive_values_not_all_equal);
    RUN_TEST(test_rng_seed_zero_is_nonzero_and_deterministic);
    RUN_TEST(test_rng_below_stays_within_bound);
    RUN_TEST(test_rng_below_one_and_zero_return_zero);
    RUN_TEST(test_rng_null_self_is_safe);
    RUN_TEST(test_rng_shifts_operate_on_uint32_no_signed_ub);
    return UNITY_END();
}
