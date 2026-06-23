#include "unity.h"
#include "hysteresis.h"

void setUp(void) {}
void tearDown(void) {}

static void test_deadband_holds(void)
{
    hysteresis h;
    hysteresis_init(&h, 20, 25, false);
    TEST_ASSERT_FALSE(hysteresis_update(&h, 22));   /* in band -> hold initial false */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 26));    /* >= high -> on */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 24));    /* in band -> hold on */
    TEST_ASSERT_FALSE(hysteresis_update(&h, 20));   /* <= low -> off */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 25));    /* >= high -> on */
}

static void test_boundary_inclusive(void)
{
    hysteresis h;
    hysteresis_init(&h, 10, 30, false);
    TEST_ASSERT_TRUE(hysteresis_update(&h, 30));    /* == high inclusive */
    TEST_ASSERT_FALSE(hysteresis_update(&h, 10));   /* == low inclusive */
}

static void test_swap(void)
{
    hysteresis h;
    hysteresis_init(&h, 25, 20, false);             /* low>high -> swapped */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 25));
    TEST_ASSERT_FALSE(hysteresis_update(&h, 20));
}

static void test_null(void)
{
    TEST_ASSERT_FALSE(hysteresis_update(NULL, 5));
    TEST_ASSERT_FALSE(hysteresis_state(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_deadband_holds);
    RUN_TEST(test_boundary_inclusive);
    RUN_TEST(test_swap);
    RUN_TEST(test_null);
    return UNITY_END();
}
