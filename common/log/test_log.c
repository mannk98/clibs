#include "unity.h"
#include "log.h"
#include <stdio.h>   /* vsnprintf */

void setUp(void) {}
void tearDown(void) {}

/* capture_cb is registered at LOG_TRACE and records the last event */
static char        cap_msg[128];
static int         cap_level;
static const char *cap_tag;

static void capture_cb(log_Event *ev) {
    cap_level = ev->level;
    cap_tag   = ev->tag;
    vsnprintf(cap_msg, sizeof(cap_msg), ev->fmt, ev->ap);
}

/* error_cb is registered at LOG_WARN to test per-callback level filtering */
static int err_calls;
static void error_cb(log_Event *ev) { (void)ev; err_calls++; }

static void test_level_string(void) {
    TEST_ASSERT_EQUAL_STRING("INFO",  log_level_string(LOG_INFO));
    TEST_ASSERT_EQUAL_STRING("FATAL", log_level_string(LOG_FATAL));
}

static void test_tag_threading_and_format(void) {
    log_info("NET", "value=%d", 7);
    TEST_ASSERT_EQUAL_INT(LOG_INFO, cap_level);
    TEST_ASSERT_EQUAL_STRING("NET", cap_tag);     /* the local fork's tag arg */
    TEST_ASSERT_EQUAL_STRING("value=7", cap_msg);
}

static void test_level_propagation(void) {
    log_warn("X", "w");
    TEST_ASSERT_EQUAL_INT(LOG_WARN, cap_level);
    log_error("X", "e");
    TEST_ASSERT_EQUAL_INT(LOG_ERROR, cap_level);
}

static void test_callback_level_filter(void) {
    int base = err_calls;
    log_info("X", "below warn");     /* LOG_INFO < LOG_WARN -> error_cb skipped */
    TEST_ASSERT_EQUAL_INT(base, err_calls);
    log_error("X", "at/above warn"); /* LOG_ERROR >= LOG_WARN -> error_cb fires */
    TEST_ASSERT_EQUAL_INT(base + 1, err_calls);
}

int main(void) {
    log_set_quiet(true);                              /* silence the stderr sink */
    log_set_level(LOG_TRACE);
    log_add_callback(capture_cb, NULL, LOG_TRACE);    /* sees everything */
    log_add_callback(error_cb,   NULL, LOG_WARN);     /* WARN and above only */
    UNITY_BEGIN();
    RUN_TEST(test_level_string);
    RUN_TEST(test_tag_threading_and_format);
    RUN_TEST(test_level_propagation);
    RUN_TEST(test_callback_level_filter);
    return UNITY_END();
}
