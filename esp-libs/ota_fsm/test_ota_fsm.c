/* RED-step Unity host test for the ota_fsm pure state machine.
 * Tests only the API contract (C16-2) — the header does not exist yet.
 * No production code is read or written here. */
#include "unity.h"
#include "ota_fsm.h"   /* pure header, freestanding (<stdint.h>, no SDK) — to be created by Dev */

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance 1: init -> state OTA_IDLE, progress 0. */
static void test_ota_fsm_init_idle_progress_zero(void) {
    ota_fsm f;
    ota_fsm_init(&f);
    TEST_ASSERT_EQUAL_INT(OTA_IDLE, ota_fsm_state(&f));
    TEST_ASSERT_EQUAL_UINT32(0, f.total);
    TEST_ASSERT_EQUAL_UINT32(0, f.received);
    TEST_ASSERT_EQUAL_UINT8(0, ota_fsm_progress(&f));
}

/* Acceptance 2: happy path START->CHECKING->DOWNLOADING->(DATA x2)->
 * DOWNLOADED->VERIFYING->VERIFIED->APPLYING->APPLIED->DONE, progress 50 mid-way. */
static void test_ota_fsm_happy_path_to_done(void) {
    ota_fsm f;
    ota_fsm_init(&f);

    TEST_ASSERT_EQUAL_INT(OTA_CHECKING, ota_fsm_on(&f, OTA_EV_START, 1000));
    TEST_ASSERT_EQUAL_UINT32(1000, f.total);
    TEST_ASSERT_EQUAL_UINT32(0, f.received);

    TEST_ASSERT_EQUAL_INT(OTA_DOWNLOADING, ota_fsm_on(&f, OTA_EV_ACCEPT, 0));

    TEST_ASSERT_EQUAL_INT(OTA_DOWNLOADING, ota_fsm_on(&f, OTA_EV_DATA, 250));
    TEST_ASSERT_EQUAL_INT(OTA_DOWNLOADING, ota_fsm_on(&f, OTA_EV_DATA, 250));
    TEST_ASSERT_EQUAL_UINT32(500, f.received);
    TEST_ASSERT_EQUAL_UINT8(50, ota_fsm_progress(&f));

    TEST_ASSERT_EQUAL_INT(OTA_VERIFYING, ota_fsm_on(&f, OTA_EV_DOWNLOADED, 0));
    TEST_ASSERT_EQUAL_INT(OTA_APPLYING,  ota_fsm_on(&f, OTA_EV_VERIFIED, 0));
    TEST_ASSERT_EQUAL_INT(OTA_DONE,      ota_fsm_on(&f, OTA_EV_APPLIED, 0));
    TEST_ASSERT_EQUAL_INT(OTA_DONE, ota_fsm_state(&f));
}

/* Acceptance 3: CHECKING on(REJECT) -> OTA_IDLE. */
static void test_ota_fsm_checking_reject_to_idle(void) {
    ota_fsm f;
    ota_fsm_init(&f);
    TEST_ASSERT_EQUAL_INT(OTA_CHECKING, ota_fsm_on(&f, OTA_EV_START, 1000));
    TEST_ASSERT_EQUAL_INT(OTA_IDLE, ota_fsm_on(&f, OTA_EV_REJECT, 0));
}

/* Acceptance 4: DOWNLOADING on(FAIL) -> OTA_FAILED. */
static void test_ota_fsm_downloading_fail_to_failed(void) {
    ota_fsm f;
    ota_fsm_init(&f);
    ota_fsm_on(&f, OTA_EV_START, 1000);
    ota_fsm_on(&f, OTA_EV_ACCEPT, 0);
    TEST_ASSERT_EQUAL_INT(OTA_DOWNLOADING, ota_fsm_state(&f));
    TEST_ASSERT_EQUAL_INT(OTA_FAILED, ota_fsm_on(&f, OTA_EV_FAIL, 0));
}

/* Acceptance 5: terminal DONE on(START,99) -> stays OTA_DONE (absorbing). */
static void test_ota_fsm_terminal_done_absorbs(void) {
    ota_fsm f;
    ota_fsm_init(&f);
    ota_fsm_on(&f, OTA_EV_START, 1000);
    ota_fsm_on(&f, OTA_EV_ACCEPT, 0);
    ota_fsm_on(&f, OTA_EV_DOWNLOADED, 0);
    ota_fsm_on(&f, OTA_EV_VERIFIED, 0);
    ota_fsm_on(&f, OTA_EV_APPLIED, 0);
    TEST_ASSERT_EQUAL_INT(OTA_DONE, ota_fsm_state(&f));
    TEST_ASSERT_EQUAL_INT(OTA_DONE, ota_fsm_on(&f, OTA_EV_START, 99));
    TEST_ASSERT_EQUAL_INT(OTA_DONE, ota_fsm_state(&f));
}

/* Acceptance 6: progress edge cases — total==0 -> 0; received>total capped at 100. */
static void test_ota_fsm_progress_edges(void) {
    /* total==0 -> 0 (divide-by-zero guard) */
    ota_fsm z;
    ota_fsm_init(&z);
    TEST_ASSERT_EQUAL_UINT8(0, ota_fsm_progress(&z));

    /* received (250) > total (100) -> capped at 100 */
    ota_fsm f;
    ota_fsm_init(&f);
    ota_fsm_on(&f, OTA_EV_START, 100);
    ota_fsm_on(&f, OTA_EV_ACCEPT, 0);
    ota_fsm_on(&f, OTA_EV_DATA, 250);
    TEST_ASSERT_EQUAL_UINT32(250, f.received);
    TEST_ASSERT_EQUAL_UINT8(100, ota_fsm_progress(&f));
}

/* Acceptance 7: NULL self — init no-op; state->OTA_FAILED; on->OTA_FAILED; progress->0. */
static void test_ota_fsm_null_self(void) {
    ota_fsm_init(NULL); /* must not crash */
    TEST_ASSERT_EQUAL_INT(OTA_FAILED, ota_fsm_state(NULL));
    TEST_ASSERT_EQUAL_INT(OTA_FAILED, ota_fsm_on(NULL, OTA_EV_START, 1000));
    TEST_ASSERT_EQUAL_UINT8(0, ota_fsm_progress(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ota_fsm_init_idle_progress_zero);
    RUN_TEST(test_ota_fsm_happy_path_to_done);
    RUN_TEST(test_ota_fsm_checking_reject_to_idle);
    RUN_TEST(test_ota_fsm_downloading_fail_to_failed);
    RUN_TEST(test_ota_fsm_terminal_done_absorbs);
    RUN_TEST(test_ota_fsm_progress_edges);
    RUN_TEST(test_ota_fsm_null_self);
    return UNITY_END();
}
