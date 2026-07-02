/* RED test for the ota_version pure-semver lib (L3, host-testable pure part).
 * Tests ONLY the API contract; the header/impl do not exist yet -> RED.
 * One test per acceptance bullet. No malloc; stack storage only. */
#include "unity.h"
#include "ota_version.h"   /* header does not exist yet -> RED */

#include <stdint.h>
#include <stdbool.h>

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: parse("1.2.3") -> true, out=={1,2,3}. */
static void test_ota_version_parse_basic(void) {
    ota_version v = {0, 0, 0};
    TEST_ASSERT_TRUE(ota_version_parse("1.2.3", &v));
    TEST_ASSERT_EQUAL_UINT16(1, v.major);
    TEST_ASSERT_EQUAL_UINT16(2, v.minor);
    TEST_ASSERT_EQUAL_UINT16(3, v.patch);
}

/* Bullet 2: parse("10.0.255") -> true, out=={10,0,255}. */
static void test_ota_version_parse_multi_digit_and_zero(void) {
    ota_version v = {0, 0, 0};
    TEST_ASSERT_TRUE(ota_version_parse("10.0.255", &v));
    TEST_ASSERT_EQUAL_UINT16(10,  v.major);
    TEST_ASSERT_EQUAL_UINT16(0,   v.minor);
    TEST_ASSERT_EQUAL_UINT16(255, v.patch);
}

/* Bullet 3: parse("1.2")/("1.2.3.4")/("1.2.x")/("") -> false (wrong shape). */
static void test_ota_version_parse_malformed_shape_is_false(void) {
    ota_version v = {0, 0, 0};
    TEST_ASSERT_FALSE(ota_version_parse("1.2",     &v)); /* too few components   */
    TEST_ASSERT_FALSE(ota_version_parse("1.2.3.4", &v)); /* too many components  */
    TEST_ASSERT_FALSE(ota_version_parse("1.2.x",   &v)); /* non-decimal char     */
    TEST_ASSERT_FALSE(ota_version_parse("",        &v)); /* empty string         */
}

/* Bullet 4: parse("1.2.99999") -> false (component > 65535). */
static void test_ota_version_parse_component_over_max_is_false(void) {
    ota_version v = {0, 0, 0};
    TEST_ASSERT_FALSE(ota_version_parse("1.2.99999", &v));
}

/* Bullet 5: parse(NULL,&v) and parse("1.2.3",NULL) -> false (no write to out). */
static void test_ota_version_parse_null_guards(void) {
    ota_version v = {7, 7, 7}; /* sentinel: must remain untouched on failure */
    TEST_ASSERT_FALSE(ota_version_parse(NULL, &v));
    TEST_ASSERT_EQUAL_UINT16(7, v.major);
    TEST_ASSERT_EQUAL_UINT16(7, v.minor);
    TEST_ASSERT_EQUAL_UINT16(7, v.patch);
    /* out==NULL must not be dereferenced and must return false. */
    TEST_ASSERT_FALSE(ota_version_parse("1.2.3", NULL));
}

/* Bullet 6: cmp lexicographic over major,minor,patch -> -1/0/1. */
static void test_ota_version_cmp_lexicographic(void) {
    ota_version a123 = {1, 2, 3};
    ota_version a124 = {1, 2, 4};
    ota_version a200 = {2, 0, 0};
    ota_version a199 = {1, 9, 9};
    TEST_ASSERT_EQUAL_INT(-1, ota_version_cmp(a123, a124)); /* patch differs   */
    TEST_ASSERT_EQUAL_INT( 1, ota_version_cmp(a200, a199)); /* major dominates  */
    TEST_ASSERT_EQUAL_INT( 0, ota_version_cmp(a123, a123)); /* equal            */
}

/* Bullet 7: is_newer true iff cmp(candidate,current) > 0. */
static void test_ota_version_is_newer(void) {
    ota_version current = {1, 2, 3};
    ota_version newer   = {1, 3, 0};
    ota_version same    = {1, 2, 3};
    ota_version older   = {1, 2, 2};
    TEST_ASSERT_TRUE (ota_version_is_newer(current, newer));
    TEST_ASSERT_FALSE(ota_version_is_newer(current, same));
    TEST_ASSERT_FALSE(ota_version_is_newer(current, older));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ota_version_parse_basic);
    RUN_TEST(test_ota_version_parse_multi_digit_and_zero);
    RUN_TEST(test_ota_version_parse_malformed_shape_is_false);
    RUN_TEST(test_ota_version_parse_component_over_max_is_false);
    RUN_TEST(test_ota_version_parse_null_guards);
    RUN_TEST(test_ota_version_cmp_lexicographic);
    RUN_TEST(test_ota_version_is_newer);
    return UNITY_END();
}
