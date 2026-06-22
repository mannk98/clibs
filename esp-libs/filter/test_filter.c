#include "unity.h"
#include "filter.h"

void setUp(void) {}
void tearDown(void) {}

static void test_first_steps(void) {
    ema f;
    ema_init(&f, 2, 0);                                  /* alpha = 1/4 */
    TEST_ASSERT_EQUAL_INT32(25, ema_update(&f, 100));    /* acc=100 -> 100>>2=25 */
    TEST_ASSERT_EQUAL_INT32(43, ema_update(&f, 100));    /* acc=175 -> 175>>2=43 */
}
static void test_converges_to_constant(void) {
    ema f;
    ema_init(&f, 3, 0);
    int32_t v = 0;
    for (int i = 0; i < 100; i++) v = ema_update(&f, 500);
    TEST_ASSERT_INT32_WITHIN(2, 500, v);
}
static void test_steady_state_stable(void) {
    ema f;
    ema_init(&f, 4, 500);                                /* start at the steady value */
    TEST_ASSERT_EQUAL_INT32(500, ema_update(&f, 500));
    TEST_ASSERT_EQUAL_INT32(500, ema_update(&f, 500));
}
static void test_null_guard(void) {
    TEST_ASSERT_EQUAL_INT32(0, ema_update(NULL, 100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_first_steps);
    RUN_TEST(test_converges_to_constant);
    RUN_TEST(test_steady_state_stable);
    RUN_TEST(test_null_guard);
    return UNITY_END();
}
