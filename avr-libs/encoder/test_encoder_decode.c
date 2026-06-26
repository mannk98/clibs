#include "unity.h"
#include "encoder_decode.h"

void setUp(void) {}
void tearDown(void) {}

static void test_cw_detent_increments(void) {
    uint8_t pre_a = 0, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 1, &counter); /* A edge 0->1, a==b, count=1 */
    TEST_ASSERT_EQUAL_INT(0, counter);              /* not yet a full detent */
    encoder_decode(&pre_a, &count, 0, 0, &counter); /* A edge 1->0, a==b, count=2 -> +1 */
    TEST_ASSERT_EQUAL_INT(1, counter);
    TEST_ASSERT_EQUAL_UINT8(0, count);              /* reset */
}
static void test_ccw_detent_decrements(void) {
    uint8_t pre_a = 0, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 0, &counter); /* A edge, a!=b, count=1 */
    encoder_decode(&pre_a, &count, 0, 1, &counter); /* A edge, a!=b, count=2 -> -1 */
    TEST_ASSERT_EQUAL_INT(-1, counter);
}
static void test_no_a_edge_no_change(void) {
    uint8_t pre_a = 1, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 0, &counter); /* A unchanged (1==1) */
    TEST_ASSERT_EQUAL_INT(0, counter);
    TEST_ASSERT_EQUAL_UINT8(0, count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cw_detent_increments);
    RUN_TEST(test_ccw_detent_decrements);
    RUN_TEST(test_no_a_edge_no_change);
    return UNITY_END();
}
