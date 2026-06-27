/* Unity host test for hashmap (open-addressing string->void* map, story CB3-1).
 *
 * RED step: tests the frozen API contract only. The implementation
 * (hashmap.c) does not exist yet, so this MUST fail (undefined references
 * to hashmap_* symbols, or a missing hashmap.h until the Dev writes it).
 * The header hashmap.h carries the contract under test.
 *
 * Contract (from the batch-3 design spec, CLIBS_HASHMAP_H):
 *   typedef struct { const char *key; void *value; } hashmap_entry;
 *   typedef struct { hashmap_entry *slots; size_t cap; size_t len; } hashmap;
 *   void   hashmap_init(hashmap *self, hashmap_entry *storage, size_t cap);
 *   bool   hashmap_put(hashmap *self, const char *key, void *value);
 *   bool   hashmap_get(const hashmap *self, const char *key, void **out);
 *   bool   hashmap_remove(hashmap *self, const char *key);
 *   size_t hashmap_len(const hashmap *self);
 *   size_t hashmap_cap(const hashmap *self);
 *
 * No malloc: storage is a caller-provided hashmap_entry array on the stack.
 * Keys are stored by pointer; the caller keeps the key strings alive
 * (string literals here have static storage duration).
 */
#include "unity.h"
#include "hashmap.h"

void setUp(void)    {}
void tearDown(void) {}

/* Distinct payload objects so we can assert which value comes out by
 * identity (pointer equality), independent of key. */
static int a, b, c;

/* Bullet 1: hashmap_entry slots[8], init(self,slots,8) -> len 0, cap 8. */
static void test_hashmap_init_empty(void)
{
    hashmap_entry slots[8];
    hashmap self;

    hashmap_init(&self, slots, 8);

    TEST_ASSERT_EQUAL_size_t(0, hashmap_len(&self));
    TEST_ASSERT_EQUAL_size_t(8, hashmap_cap(&self));
}

/* Bullet 2: put("foo",&a) + put("bar",&b) -> both true, len 2. */
static void test_hashmap_put_two_keys_len_two(void)
{
    hashmap_entry slots[8];
    hashmap self;

    hashmap_init(&self, slots, 8);

    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &a));
    TEST_ASSERT_TRUE(hashmap_put(&self, "bar", &b));
    TEST_ASSERT_EQUAL_size_t(2, hashmap_len(&self));
}

/* Bullet 3: get("foo",&out) -> true, out==&a; get("bar")-> &b;
 * get("baz") false. */
static void test_hashmap_get_present_and_absent(void)
{
    hashmap_entry slots[8];
    hashmap self;
    void *out = NULL;

    hashmap_init(&self, slots, 8);
    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &a));
    TEST_ASSERT_TRUE(hashmap_put(&self, "bar", &b));

    out = NULL;
    TEST_ASSERT_TRUE(hashmap_get(&self, "foo", &out));
    TEST_ASSERT_EQUAL_PTR(&a, out);

    out = NULL;
    TEST_ASSERT_TRUE(hashmap_get(&self, "bar", &out));
    TEST_ASSERT_EQUAL_PTR(&b, out);

    out = &c; /* must stay untouched on a miss */
    TEST_ASSERT_FALSE(hashmap_get(&self, "baz", &out));
}

/* Bullet 4: put("foo",&c) update -> true, len stays 2, get("foo")-> &c. */
static void test_hashmap_put_updates_existing_key(void)
{
    hashmap_entry slots[8];
    hashmap self;
    void *out = NULL;

    hashmap_init(&self, slots, 8);
    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &a));
    TEST_ASSERT_TRUE(hashmap_put(&self, "bar", &b));
    TEST_ASSERT_EQUAL_size_t(2, hashmap_len(&self));

    /* Update existing key: returns true, len unchanged. */
    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &c));
    TEST_ASSERT_EQUAL_size_t(2, hashmap_len(&self));

    out = NULL;
    TEST_ASSERT_TRUE(hashmap_get(&self, "foo", &out));
    TEST_ASSERT_EQUAL_PTR(&c, out);
}

/* Bullet 5: remove("foo") -> true, len 1, get("foo") false;
 * remove("foo") again -> false. */
static void test_hashmap_remove_then_absent(void)
{
    hashmap_entry slots[8];
    hashmap self;
    void *out = NULL;

    hashmap_init(&self, slots, 8);
    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &a));
    TEST_ASSERT_TRUE(hashmap_put(&self, "bar", &b));

    TEST_ASSERT_TRUE(hashmap_remove(&self, "foo"));
    TEST_ASSERT_EQUAL_size_t(1, hashmap_len(&self));

    out = NULL;
    TEST_ASSERT_FALSE(hashmap_get(&self, "foo", &out));

    /* "bar" survives the remove of "foo" (probe chain intact). */
    out = NULL;
    TEST_ASSERT_TRUE(hashmap_get(&self, "bar", &out));
    TEST_ASSERT_EQUAL_PTR(&b, out);

    /* Removing an absent key returns false. */
    TEST_ASSERT_FALSE(hashmap_remove(&self, "foo"));
}

/* Bullet 6: insert 6 distinct keys in 8-slot map -> all 6 retrievable
 * (linear probing + tombstones). */
static void test_hashmap_six_keys_all_retrievable(void)
{
    hashmap_entry slots[8];
    hashmap self;
    void *out = NULL;

    /* Six distinct values to assert identity per key. */
    static int v0, v1, v2, v3, v4, v5;
    static const char *const keys[6] = {
        "alpha", "bravo", "charlie", "delta", "echo", "foxtrot"
    };
    int *const vals[6] = { &v0, &v1, &v2, &v3, &v4, &v5 };
    int i;

    hashmap_init(&self, slots, 8);

    for (i = 0; i < 6; i++) {
        TEST_ASSERT_TRUE(hashmap_put(&self, keys[i], vals[i]));
    }
    TEST_ASSERT_EQUAL_size_t(6, hashmap_len(&self));

    for (i = 0; i < 6; i++) {
        out = NULL;
        TEST_ASSERT_TRUE(hashmap_get(&self, keys[i], &out));
        TEST_ASSERT_EQUAL_PTR(vals[i], out);
    }
}

/* Bullet 7: fill until no empty/tombstone slot remains -> put new key
 * returns false (full). */
static void test_hashmap_put_when_full_returns_false(void)
{
    hashmap_entry slots[8];
    hashmap self;
    static int vals[8];
    static const char *const keys[8] = {
        "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7"
    };
    int i;

    hashmap_init(&self, slots, 8);

    /* Fill all 8 slots. */
    for (i = 0; i < 8; i++) {
        TEST_ASSERT_TRUE(hashmap_put(&self, keys[i], &vals[i]));
    }
    TEST_ASSERT_EQUAL_size_t(8, hashmap_len(&self));

    /* No empty/tombstone slot remains: a new key cannot be inserted. */
    TEST_ASSERT_FALSE(hashmap_put(&self, "overflow", &a));
    TEST_ASSERT_EQUAL_size_t(8, hashmap_len(&self));

    /* Updating an EXISTING key in a full map still works (len unchanged). */
    TEST_ASSERT_TRUE(hashmap_put(&self, "k0", &b));
    TEST_ASSERT_EQUAL_size_t(8, hashmap_len(&self));
}

/* Bullet 8: get(self,"foo",NULL) -> true (membership test, no deref of out). */
static void test_hashmap_get_membership_null_out(void)
{
    hashmap_entry slots[8];
    hashmap self;

    hashmap_init(&self, slots, 8);
    TEST_ASSERT_TRUE(hashmap_put(&self, "foo", &a));

    /* out == NULL: pure membership test, returns true without writing. */
    TEST_ASSERT_TRUE(hashmap_get(&self, "foo", NULL));
    /* Absent key with NULL out -> false. */
    TEST_ASSERT_FALSE(hashmap_get(&self, "baz", NULL));
}

/* Bullet 9: NULL/edge:
 *   put(NULL,..) false; put(self,NULL,..) false;
 *   get(NULL,..) false; remove(NULL,..) false;
 *   len(NULL) 0; cap(NULL) 0. */
static void test_hashmap_null_edge_safe(void)
{
    hashmap_entry slots[8];
    hashmap self;
    void *out = NULL;

    hashmap_init(&self, slots, 8);

    /* NULL self / NULL key are all safe, defined returns. */
    TEST_ASSERT_FALSE(hashmap_put(NULL, "foo", &a));
    TEST_ASSERT_FALSE(hashmap_put(&self, NULL, &a));

    TEST_ASSERT_FALSE(hashmap_get(NULL, "foo", &out));
    TEST_ASSERT_FALSE(hashmap_get(&self, NULL, &out));

    TEST_ASSERT_FALSE(hashmap_remove(NULL, "foo"));
    TEST_ASSERT_FALSE(hashmap_remove(&self, NULL));

    TEST_ASSERT_EQUAL_size_t(0, hashmap_len(NULL));
    TEST_ASSERT_EQUAL_size_t(0, hashmap_cap(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_init_empty);
    RUN_TEST(test_hashmap_put_two_keys_len_two);
    RUN_TEST(test_hashmap_get_present_and_absent);
    RUN_TEST(test_hashmap_put_updates_existing_key);
    RUN_TEST(test_hashmap_remove_then_absent);
    RUN_TEST(test_hashmap_six_keys_all_retrievable);
    RUN_TEST(test_hashmap_put_when_full_returns_false);
    RUN_TEST(test_hashmap_get_membership_null_out);
    RUN_TEST(test_hashmap_null_edge_safe);
    return UNITY_END();
}
