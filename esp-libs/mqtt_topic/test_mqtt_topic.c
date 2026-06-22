#include "unity.h"
#include "mqtt_topic.h"

void setUp(void) {}
void tearDown(void) {}

static void test_join_basic(void) {
    const char *parts[] = { "home", "kitchen", "temp" };
    char buf[32];
    TEST_ASSERT_EQUAL_INT(17, mqtt_topic_join(buf, sizeof buf, parts, 3));
    TEST_ASSERT_EQUAL_STRING("home/kitchen/temp", buf);
}
static void test_join_empty(void) {
    char buf[8];
    TEST_ASSERT_EQUAL_INT(0, mqtt_topic_join(buf, sizeof buf, NULL, 0));
    TEST_ASSERT_EQUAL_STRING("", buf);
}
static void test_join_truncation(void) {
    const char *parts[] = { "home", "kitchen", "temp" };
    char buf[8];
    TEST_ASSERT_EQUAL_INT(-1, mqtt_topic_join(buf, sizeof buf, parts, 3));
    TEST_ASSERT_EQUAL_STRING("", buf);
}
static void test_match_plus(void) {
    TEST_ASSERT_TRUE (mqtt_topic_match("home/+/temp", "home/kitchen/temp"));
    TEST_ASSERT_FALSE(mqtt_topic_match("home/+/temp", "home/a/b/temp"));
    TEST_ASSERT_TRUE (mqtt_topic_match("+", "a"));
    TEST_ASSERT_FALSE(mqtt_topic_match("+", "a/b"));
}
static void test_match_hash(void) {
    TEST_ASSERT_TRUE(mqtt_topic_match("home/#", "home/a/b"));
    TEST_ASSERT_TRUE(mqtt_topic_match("home/#", "home"));   /* parent match */
    TEST_ASSERT_TRUE(mqtt_topic_match("#", "anything/at/all"));
}
static void test_match_exact(void) {
    TEST_ASSERT_TRUE (mqtt_topic_match("a/b", "a/b"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b", "a/b/c"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b", "a"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b/c", "a/b"));
}
static void test_null_guard(void) {
    TEST_ASSERT_FALSE(mqtt_topic_match(NULL, "a"));
    TEST_ASSERT_EQUAL_INT(-1, mqtt_topic_join(NULL, 10, NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_join_basic);
    RUN_TEST(test_join_empty);
    RUN_TEST(test_join_truncation);
    RUN_TEST(test_match_plus);
    RUN_TEST(test_match_hash);
    RUN_TEST(test_match_exact);
    RUN_TEST(test_null_guard);
    return UNITY_END();
}
