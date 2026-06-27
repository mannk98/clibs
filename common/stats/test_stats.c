/* Unity host test for `stats` (Welford streaming min/max/mean/variance) — RED step.
 *
 * Tests ONLY the frozen API contract from the batch-3 design spec
 * (docs/superpowers/specs/2026-06-27-clibs-common-oop-batch3-design.md). The
 * implementation (stats.c) does not exist yet, so the link MUST fail with an
 * undefined-reference error (or, once stubbed, an unmet assertion). No malloc:
 * the `stats` object lives on the stack. Population variance = m2/count.
 *
 * Contract under test:
 *   void    stats_init(stats *self);
 *   void    stats_add(stats *self, int32_t x);
 *   size_t  stats_count(const stats *self);
 *   int32_t stats_min(const stats *self);        // 0 if count==0
 *   int32_t stats_max(const stats *self);        // 0 if count==0
 *   float   stats_mean(const stats *self);       // 0.0f if count==0
 *   float   stats_variance(const stats *self);   // m2/count; 0.0f if count==0
 */
#include "unity.h"
#include "stats.h"

void setUp(void) {}
void tearDown(void) {}

/* Acceptance 1:
 *   init -> count 0, min 0, max 0, mean 0.0f, variance 0.0f.
 */
static void test_stats_init_zeroes_all(void) {
    stats s;
    stats_init(&s);
    TEST_ASSERT_EQUAL_size_t(0, stats_count(&s));
    TEST_ASSERT_EQUAL_INT32(0, stats_min(&s));
    TEST_ASSERT_EQUAL_INT32(0, stats_max(&s));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stats_mean(&s));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stats_variance(&s));
}

/* Acceptance 2:
 *   add 2,4,6 -> count 3, min 2, max 6, mean ~4.0f,
 *   variance ~2.6667f (population m2/count).
 */
static void test_stats_add_2_4_6(void) {
    stats s;
    stats_init(&s);
    stats_add(&s, 2);
    stats_add(&s, 4);
    stats_add(&s, 6);
    TEST_ASSERT_EQUAL_size_t(3, stats_count(&s));
    TEST_ASSERT_EQUAL_INT32(2, stats_min(&s));
    TEST_ASSERT_EQUAL_INT32(6, stats_max(&s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, stats_mean(&s));
    /* population variance = ((2-4)^2 + (4-4)^2 + (6-4)^2) / 3 = 8/3 = 2.6667 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.6667f, stats_variance(&s));
}

/* Acceptance 3:
 *   add 5 once -> count 1, min 5, max 5, mean ~5.0f, variance 0.0f.
 */
static void test_stats_add_single_5(void) {
    stats s;
    stats_init(&s);
    stats_add(&s, 5);
    TEST_ASSERT_EQUAL_size_t(1, stats_count(&s));
    TEST_ASSERT_EQUAL_INT32(5, stats_min(&s));
    TEST_ASSERT_EQUAL_INT32(5, stats_max(&s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, stats_mean(&s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats_variance(&s));
}

/* Acceptance 4:
 *   add 5,1,9,3 -> min 1, max 9, mean ~4.5f.
 */
static void test_stats_add_5_1_9_3(void) {
    stats s;
    stats_init(&s);
    stats_add(&s, 5);
    stats_add(&s, 1);
    stats_add(&s, 9);
    stats_add(&s, 3);
    TEST_ASSERT_EQUAL_size_t(4, stats_count(&s));
    TEST_ASSERT_EQUAL_INT32(1, stats_min(&s));
    TEST_ASSERT_EQUAL_INT32(9, stats_max(&s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.5f, stats_mean(&s));
}

/* Acceptance 5 — Welford update law:
 *   count++; d=x-mean; mean+=d/count; m2+=d*(x-mean); variance=m2/count.
 * Verified observationally via an independent reference computation on a
 * fresh data set {10, 20, 30, 40} (mean 25; population variance
 * = (225+25+25+225)/4 = 500/4 = 125). The running mean after each add must
 * track the Welford incremental formula, so we also check the partial mean
 * after the first two adds (mean of {10,20} = 15).
 */
static void test_stats_welford_update_law(void) {
    stats s;
    stats_init(&s);
    stats_add(&s, 10);
    stats_add(&s, 20);
    /* partial: after two values the running mean is 15 (Welford increment) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, stats_mean(&s));
    stats_add(&s, 30);
    stats_add(&s, 40);
    TEST_ASSERT_EQUAL_size_t(4, stats_count(&s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, stats_mean(&s));
    /* population variance m2/count = 500/4 = 125 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 125.0f, stats_variance(&s));
}

/* Acceptance 6 — NULL/edge safety:
 *   stats_add(NULL,x) no crash; stats_count(NULL) 0; stats_mean(NULL) 0.0f;
 *   stats_variance(NULL) 0.0f; stats_min/max(NULL) 0.
 */
static void test_stats_null_self_safe(void) {
    stats_add(NULL, 42); /* must not crash */
    TEST_ASSERT_EQUAL_size_t(0, stats_count(NULL));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stats_mean(NULL));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stats_variance(NULL));
    TEST_ASSERT_EQUAL_INT32(0, stats_min(NULL));
    TEST_ASSERT_EQUAL_INT32(0, stats_max(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stats_init_zeroes_all);
    RUN_TEST(test_stats_add_2_4_6);
    RUN_TEST(test_stats_add_single_5);
    RUN_TEST(test_stats_add_5_1_9_3);
    RUN_TEST(test_stats_welford_update_law);
    RUN_TEST(test_stats_null_self_safe);
    return UNITY_END();
}
