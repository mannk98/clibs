/* RED test for the ota_manifest pure validator (L3, host-testable pure part).
 * Tests ONLY the API contract; the header/impl do not exist yet -> RED.
 * One test per acceptance bullet. No malloc; stack storage only.
 *
 * Contract under test:
 *   bool ota_manifest_valid(const char *version, const char *url,
 *                           uint32_t size, const char *sha256_hex,
 *                           uint32_t max_size);
 * true iff:
 *   - ota_version_parse(version) succeeds (so links ota_version.c);
 *   - strncmp(url,"http://",7)==0 && url[7]!='\0';
 *   - 0 < size <= max_size;
 *   - strlen(sha256_hex)==64 && all chars in [0-9a-f] (lowercase only);
 *   - any NULL pointer arg -> false. */
#include "unity.h"
#include "ota_manifest.h"   /* header does not exist yet -> RED */

#include <stdint.h>
#include <stdbool.h>

void setUp(void) {}
void tearDown(void) {}

/* A canonical 64-char lowercase-hex sha256 digest used as the "good" value. */
#define GOOD_SHA "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#define MAX_SIZE ((uint32_t)1048576u)  /* 1 MiB */

/* Bullet 1 (accept anchor): fully-valid manifest -> true. */
static void test_ota_manifest_valid_accept_anchor(void) {
    TEST_ASSERT_TRUE(ota_manifest_valid("1.3.0",
                                        "http://192.168.4.1/fw.bin",
                                        421000u, GOOD_SHA, MAX_SIZE));
}

/* Bullet 2: size boundaries. 0 -> false; max+1 -> false; ==max -> true. */
static void test_ota_manifest_valid_size_bounds(void) {
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "http://h/f.bin",
                                         0u, GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "http://h/f.bin",
                                         MAX_SIZE + 1u, GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_TRUE(ota_manifest_valid("1.3.0", "http://h/f.bin",
                                        MAX_SIZE, GOOD_SHA, MAX_SIZE));
}

/* Bullet 3: sha256 must be exactly 64 lowercase-hex chars.
 * 63 chars -> false; 64 with a 'G' -> false; 64 uppercase 'A' -> false. */
static void test_ota_manifest_valid_sha256_rules(void) {
    /* 63 chars (one short). */
    TEST_ASSERT_FALSE(ota_manifest_valid(
        "1.3.0", "http://h/f.bin", 100u,
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde",
        MAX_SIZE));
    /* 64 chars but contains a non-hex 'G'. */
    TEST_ASSERT_FALSE(ota_manifest_valid(
        "1.3.0", "http://h/f.bin", 100u,
        "G123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
        MAX_SIZE));
    /* 64 chars, all uppercase hex 'A' -> lowercase-only rule rejects. */
    TEST_ASSERT_FALSE(ota_manifest_valid(
        "1.3.0", "http://h/f.bin", 100u,
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        MAX_SIZE));
}

/* Bullet 4: url must start with "http://" (7 chars) AND have more after it.
 * "https://x" -> false; "ftp://x" -> false; "http://" (len 7) -> false. */
static void test_ota_manifest_valid_url_rules(void) {
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "https://x", 100u,
                                         GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "ftp://x", 100u,
                                         GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "http://", 100u,
                                         GOOD_SHA, MAX_SIZE));
}

/* Bullet 5: version must parse as full semver. "1.2" -> false. */
static void test_ota_manifest_valid_version_must_parse(void) {
    TEST_ASSERT_FALSE(ota_manifest_valid("1.2", "http://h/f.bin", 100u,
                                         GOOD_SHA, MAX_SIZE));
}

/* Bullet 6: any NULL pointer arg -> false. */
static void test_ota_manifest_valid_null_guards(void) {
    TEST_ASSERT_FALSE(ota_manifest_valid(NULL, "http://h/f.bin", 100u,
                                         GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", NULL, 100u,
                                         GOOD_SHA, MAX_SIZE));
    TEST_ASSERT_FALSE(ota_manifest_valid("1.3.0", "http://h/f.bin", 100u,
                                         NULL, MAX_SIZE));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ota_manifest_valid_accept_anchor);
    RUN_TEST(test_ota_manifest_valid_size_bounds);
    RUN_TEST(test_ota_manifest_valid_sha256_rules);
    RUN_TEST(test_ota_manifest_valid_url_rules);
    RUN_TEST(test_ota_manifest_valid_version_must_parse);
    RUN_TEST(test_ota_manifest_valid_null_guards);
    return UNITY_END();
}
