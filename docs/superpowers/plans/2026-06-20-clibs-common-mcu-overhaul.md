# clibs `common/` MCU-Lightweight Overhaul — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite the clibs-owned portable C libraries (`dll`, `ringbuf`, `str_utils`) to be malloc-free, statically-allocated, OOP-in-C, and unit-tested with Unity; light-touch fix + test the `log` fork.

**Architecture:** Each container is a transparent struct + `module_<verb>(self, …)` methods over **caller-provided storage** (no heap). Tests run on the host with Unity (`make test`); the reusable libs compile clean under `-std=c11 -Wall -Wextra -Werror` and use only freestanding headers so they cross-compile for AVR unchanged.

**Tech Stack:** C11/C99, Unity (ThrowTheSwitch, vendored), GNU make, host `cc`/`gcc`.

**Spec:** `docs/superpowers/specs/2026-06-20-clibs-common-mcu-overhaul-design.md`

**Note on commits:** `common/` is committed directly in the `clibs` repo (not a submodule), so every commit below is an ordinary `clibs` commit. The repo uses the personal git identity automatically (includeIf). All work happens under `/Users/man.nk/git/clibs/common/`.

---

## File Structure

| File | Responsibility |
|------|----------------|
| `common/third_party/unity/unity.{c,h}`, `unity_internals.h` | Vendored Unity test framework |
| `common/Makefile` | Build + run each host test suite; `strict` compile gate; `clean` |
| `common/str_utils/str_utils.{h,c}` | Freestanding string helpers |
| `common/str_utils/test_str_utils.c` | Unity suite for str_utils |
| `common/ringbuf/ringbuf.{h,c}` | Generic flat-array FIFO over caller storage |
| `common/ringbuf/test_ringbuf.c` | Unity suite for ringbuf |
| `common/dll/dll.{h,c}` | Doubly linked list over a caller node pool |
| `common/dll/test_dll.c` | Unity suite for dll |
| `common/log/src/log.{c,h}`, `example/main.c` | rxi log fork (moved from `log.c/`), light-touch fixed |
| `common/log/test_log.c` | Unity suite for log |
| `common/README.md` | Rewritten overview of the new layout |

Removed at the end: `common/dlinklist.c/`, `common/ringbuffchar.c/`, all `Debug/` dirs and tracked ELF binaries.

---

## Task 1: Scaffold — directories, Unity, Makefile, move `log`

**Files:**
- Create: `common/str_utils/`, `common/ringbuf/`, `common/dll/`, `common/third_party/unity/`
- Create: `common/third_party/unity/unity.c`, `unity.h`, `unity_internals.h` (downloaded)
- Create: `common/Makefile`
- Move: `common/log.c/` → `common/log/` (git mv, preserves history)

- [ ] **Step 1: Create the new directories**

Run:
```bash
cd /Users/man.nk/git/clibs/common
mkdir -p str_utils ringbuf dll third_party/unity build
```

- [ ] **Step 2: Move the log folder to its clean name (preserve history)**

Run:
```bash
cd /Users/man.nk/git/clibs/common
git mv log.c log
ls log            # expect: example  src  LICENSE
```

- [ ] **Step 3: Vendor Unity (3 files) from the pinned release**

Run:
```bash
cd /Users/man.nk/git/clibs/common/third_party/unity
BASE=https://raw.githubusercontent.com/ThrowTheSwitch/Unity/v2.6.0/src
curl -fsSL $BASE/unity.c -o unity.c
curl -fsSL $BASE/unity.h -o unity.h
curl -fsSL $BASE/unity_internals.h -o unity_internals.h
ls -1            # expect: unity.c  unity.h  unity_internals.h
```
Expected: three non-empty files. If a URL 404s, replace `v2.6.0` with `master`.

- [ ] **Step 4: Write the Makefile**

Create `common/Makefile`:
```makefile
# Host build + test runner for the clibs portable libraries.
CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -g
UNITY   := third_party/unity
BUILD   := build

.PHONY: test str_utils ringbuf dll log strict clean
test: str_utils ringbuf dll log

$(BUILD):
	mkdir -p $(BUILD)

str_utils: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Istr_utils -o $(BUILD)/test_str_utils \
	    str_utils/test_str_utils.c str_utils/str_utils.c $(UNITY)/unity.c
	./$(BUILD)/test_str_utils

ringbuf: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iringbuf -o $(BUILD)/test_ringbuf \
	    ringbuf/test_ringbuf.c ringbuf/ringbuf.c $(UNITY)/unity.c
	./$(BUILD)/test_ringbuf

dll: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idll -o $(BUILD)/test_dll \
	    dll/test_dll.c dll/dll.c $(UNITY)/unity.c
	./$(BUILD)/test_dll

log: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ilog/src -o $(BUILD)/test_log \
	    log/test_log.c log/src/log.c $(UNITY)/unity.c
	./$(BUILD)/test_log

# Strict compile gate: reusable libs must be warning-clean under -Werror.
# log is compiled WITHOUT -Werror (third-party style) but WITH color disabled
# to exercise its non-color branch (catches the tag bug).
strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Istr_utils -c str_utils/str_utils.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iringbuf   -c ringbuf/ringbuf.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idll       -c dll/dll.c             -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -DLOG_DISABLE_COLOR -Ilog/src -c log/src/log.c -o /dev/null
	@echo "strict: OK"

clean:
	rm -rf $(BUILD)
```

- [ ] **Step 5: Add a .gitignore for the build dir**

Create `common/.gitignore`:
```
build/
```

- [ ] **Step 6: Commit the scaffold**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/Makefile common/.gitignore common/third_party/unity common/log
git commit -m "build(common): vendor Unity, add Makefile, rename log.c/ -> log/"
```

---

## Task 2: `str_utils` (smallest, no deps) — TDD

**Files:**
- Create: `common/str_utils/str_utils.h`, `common/str_utils/str_utils.c`
- Test: `common/str_utils/test_str_utils.c`

- [ ] **Step 1: Write the header**

Create `common/str_utils/str_utils.h`:
```c
#ifndef CLIBS_STR_UTILS_H
#define CLIBS_STR_UTILS_H

#include <stdint.h>

/* Freestanding string helpers — no libc <string.h> dependency (MCU-friendly). */

/* Length of NUL-terminated string s (0 if s is NULL). */
uint16_t str_len(const char *s);

/* strncmp-style: compare up to n bytes. Returns 0 if equal, else the signed
 * difference of the first differing byte (compared as unsigned char). No
 * truncation. a and b must be non-NULL. */
int str_compare_n(const char *a, const char *b, uint16_t n);

/* Safe bounded copy (strlcpy-style): copies at most n bytes from src to dst
 * INCLUDING the terminating NUL; dst is always NUL-terminated when n > 0.
 * Returns dst. */
char *str_cpy_n(char *dst, const char *src, uint16_t n);

/* Reverse s in place (uses str_len, not libc strlen). No-op if s is NULL or len<2. */
void str_reverse(char *s);

#endif /* CLIBS_STR_UTILS_H */
```

- [ ] **Step 2: Write the failing test**

Create `common/str_utils/test_str_utils.c`:
```c
#include "unity.h"
#include "str_utils.h"

void setUp(void) {}
void tearDown(void) {}

static void test_str_len(void) {
    TEST_ASSERT_EQUAL_UINT16(0, str_len(""));
    TEST_ASSERT_EQUAL_UINT16(5, str_len("hello"));
    TEST_ASSERT_EQUAL_UINT16(0, str_len(NULL));
}

static void test_str_compare_n_equal_and_prefix(void) {
    TEST_ASSERT_EQUAL_INT(0, str_compare_n("abc", "abc", 3));
    TEST_ASSERT_EQUAL_INT(0, str_compare_n("abc", "abd", 2)); /* first 2 equal */
    TEST_ASSERT_TRUE(str_compare_n("abc", "abd", 3) < 0);     /* 'c' < 'd'      */
}

static void test_str_compare_n_no_uint8_truncation(void) {
    /* 'a'(97) vs 0x80(128): as unsigned char, 'a' < 0x80 -> negative. The old
     * uint8_t return would mangle this sign. */
    char hi[2] = { (char)0x80, 0 };
    char lo[2] = { 'a', 0 };
    TEST_ASSERT_TRUE(str_compare_n(lo, hi, 1) < 0);
}

static void test_str_cpy_n_truncates_and_terminates(void) {
    char dst[4] = { 'Z', 'Z', 'Z', 'Z' };
    str_cpy_n(dst, "hello", 4);
    TEST_ASSERT_EQUAL_STRING("hel", dst); /* 3 chars + NUL, never overruns */
}

static void test_str_cpy_n_fits(void) {
    char dst[8];
    str_cpy_n(dst, "hi", 8);
    TEST_ASSERT_EQUAL_STRING("hi", dst);
}

static void test_str_reverse(void) {
    char s[] = "abcd";
    str_reverse(s);
    TEST_ASSERT_EQUAL_STRING("dcba", s);
    char one[] = "x";
    str_reverse(one);
    TEST_ASSERT_EQUAL_STRING("x", one);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_str_len);
    RUN_TEST(test_str_compare_n_equal_and_prefix);
    RUN_TEST(test_str_compare_n_no_uint8_truncation);
    RUN_TEST(test_str_cpy_n_truncates_and_terminates);
    RUN_TEST(test_str_cpy_n_fits);
    RUN_TEST(test_str_reverse);
    return UNITY_END();
}
```

- [ ] **Step 3: Run the test to verify it fails (no implementation yet)**

Run: `cd /Users/man.nk/git/clibs/common && make str_utils`
Expected: link error / undefined references to `str_len`, `str_compare_n`, etc. (no `str_utils.c` yet).

- [ ] **Step 4: Write the implementation**

Create `common/str_utils/str_utils.c`:
```c
#include "str_utils.h"
#include <stddef.h>  /* NULL */

uint16_t str_len(const char *s) {
    uint16_t n = 0;
    if (s == NULL) return 0;
    while (s[n] != '\0') n++;
    return n;
}

int str_compare_n(const char *a, const char *b, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return (int)ca - (int)cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

char *str_cpy_n(char *dst, const char *src, uint16_t n) {
    if (dst == NULL || n == 0) return dst;
    uint16_t i = 0;
    if (src != NULL) {
        for (; i < (uint16_t)(n - 1) && src[i] != '\0'; i++)
            dst[i] = src[i];
    }
    dst[i] = '\0';
    return dst;
}

void str_reverse(char *s) {
    if (s == NULL) return;
    uint16_t len = str_len(s);
    if (len < 2) return;
    uint16_t i = 0, j = (uint16_t)(len - 1);
    while (i < j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
}
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd /Users/man.nk/git/clibs/common && make str_utils`
Expected: Unity prints `6 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Commit**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/str_utils
git commit -m "feat(common/str_utils): freestanding string helpers + tests

Fix str_compare_n int truncation and str_reverse implicit strlen."
```

---

## Task 3: `ringbuf` — generic flat-array FIFO — TDD

**Files:**
- Create: `common/ringbuf/ringbuf.h`, `common/ringbuf/ringbuf.c`
- Test: `common/ringbuf/test_ringbuf.c`

- [ ] **Step 1: Write the header**

Create `common/ringbuf/ringbuf.h`:
```c
#ifndef CLIBS_RINGBUF_H
#define CLIBS_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Generic fixed-capacity FIFO over caller-provided storage. No malloc.
 * Value semantics: elements are copied in/out (memcpy of elem_size bytes).
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli. */

typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;

typedef struct {
    uint8_t *buf;       /* storage, must be at least cap*elem_size bytes */
    uint16_t cap;       /* element capacity */
    uint16_t elem_size; /* bytes per element (1 = classic byte/UART buffer) */
    uint16_t head;      /* index of oldest element */
    uint16_t count;     /* elements currently stored */
    bool     overwrite; /* when full: evict oldest (true) vs reject (false) */
} ringbuf;

void     ringbuf_init(ringbuf *self, void *storage, uint16_t cap,
                      uint16_t elem_size, bool overwrite);
rb_err   ringbuf_put(ringbuf *self, const void *elem);
rb_err   ringbuf_get(ringbuf *self, void *out);   /* out may be NULL to just drop */
rb_err   ringbuf_peek(const ringbuf *self, void *out);
uint16_t ringbuf_count(const ringbuf *self);
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full(const ringbuf *self);
void     ringbuf_clear(ringbuf *self);

#endif /* CLIBS_RINGBUF_H */
```

- [ ] **Step 2: Write the failing test**

Create `common/ringbuf/test_ringbuf.c`:
```c
#include "unity.h"
#include "ringbuf.h"

void setUp(void) {}
void tearDown(void) {}

static void test_fifo_order_bytes(void) {
    uint8_t store[4];
    ringbuf rb;
    ringbuf_init(&rb, store, 4, sizeof(uint8_t), false);
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));

    uint8_t in;
    in = 10; TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, &in));
    in = 20; TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, &in));
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));

    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(10, out);              /* oldest first */
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(20, out);
    TEST_ASSERT_EQUAL_INT(RB_EMPTY, ringbuf_get(&rb, &out));
}

static void test_full_reject_when_no_overwrite(void) {
    uint8_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, 1, false);
    uint8_t v = 1;
    ringbuf_put(&rb, &v);
    ringbuf_put(&rb, &v);
    TEST_ASSERT_TRUE(ringbuf_is_full(&rb));
    TEST_ASSERT_EQUAL_INT(RB_FULL, ringbuf_put(&rb, &v));
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));
}

static void test_overwrite_evicts_oldest(void) {
    uint8_t store[3];
    ringbuf rb;
    ringbuf_init(&rb, store, 3, 1, true);
    for (uint8_t i = 1; i <= 3; i++) ringbuf_put(&rb, &i);
    uint8_t four = 4;
    TEST_ASSERT_EQUAL_INT(RB_OVERWRITE, ringbuf_put(&rb, &four)); /* drops 1 */
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);      /* 1 was evicted */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_wraparound(void) {
    uint8_t store[3];
    ringbuf rb;
    ringbuf_init(&rb, store, 3, 1, false);
    uint8_t v, out;
    v = 1; ringbuf_put(&rb, &v);
    v = 2; ringbuf_put(&rb, &v);
    ringbuf_get(&rb, &out);            /* drop 1, head advances */
    v = 3; ringbuf_put(&rb, &v);
    v = 4; ringbuf_put(&rb, &v);       /* wraps into freed slot */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_peek_does_not_remove(void) {
    uint8_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, 1, false);
    uint8_t v = 99; ringbuf_put(&rb, &v);
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_peek(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(99, out);
    TEST_ASSERT_EQUAL_UINT16(1, ringbuf_count(&rb)); /* still there */
}

static void test_multibyte_elements(void) {
    uint32_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, sizeof(uint32_t), false);
    uint32_t in = 0xDEADBEEF, out = 0;
    ringbuf_put(&rb, &in);
    ringbuf_get(&rb, &out);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, out);
}

static void test_invalid_args(void) {
    ringbuf rb;
    uint8_t store[1];
    ringbuf_init(&rb, store, 1, 1, false);
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(&rb, NULL));
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(NULL, store));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fifo_order_bytes);
    RUN_TEST(test_full_reject_when_no_overwrite);
    RUN_TEST(test_overwrite_evicts_oldest);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_peek_does_not_remove);
    RUN_TEST(test_multibyte_elements);
    RUN_TEST(test_invalid_args);
    return UNITY_END();
}
```

- [ ] **Step 3: Run the test to verify it fails**

Run: `cd /Users/man.nk/git/clibs/common && make ringbuf`
Expected: undefined references to `ringbuf_init`, `ringbuf_put`, etc.

- [ ] **Step 4: Write the implementation**

Create `common/ringbuf/ringbuf.c`:
```c
#include "ringbuf.h"
#include <string.h>  /* memcpy */

void ringbuf_init(ringbuf *self, void *storage, uint16_t cap,
                  uint16_t elem_size, bool overwrite) {
    if (self == NULL) return;
    self->buf = (uint8_t *)storage;
    self->cap = cap;
    self->elem_size = elem_size;
    self->head = 0;
    self->count = 0;
    self->overwrite = overwrite;
}

rb_err ringbuf_put(ringbuf *self, const void *elem) {
    if (self == NULL || elem == NULL || self->buf == NULL || self->cap == 0)
        return RB_INVALID;

    rb_err rc = RB_OK;
    if (self->count == self->cap) {
        if (!self->overwrite) return RB_FULL;
        self->head = (uint16_t)((self->head + 1) % self->cap); /* evict oldest */
        self->count--;
        rc = RB_OVERWRITE;
    }
    uint16_t tail = (uint16_t)((self->head + self->count) % self->cap);
    memcpy(self->buf + (uint32_t)tail * self->elem_size, elem, self->elem_size);
    self->count++;
    return rc;
}

rb_err ringbuf_get(ringbuf *self, void *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL)
        memcpy(out, self->buf + (uint32_t)self->head * self->elem_size,
               self->elem_size);
    self->head = (uint16_t)((self->head + 1) % self->cap);
    self->count--;
    return RB_OK;
}

rb_err ringbuf_peek(const ringbuf *self, void *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL)
        memcpy(out, self->buf + (uint32_t)self->head * self->elem_size,
               self->elem_size);
    return RB_OK;
}

uint16_t ringbuf_count(const ringbuf *self)    { return self ? self->count : 0; }
bool     ringbuf_is_empty(const ringbuf *self) { return self ? self->count == 0 : true; }
bool     ringbuf_is_full(const ringbuf *self)  { return self ? self->count == self->cap : true; }

void ringbuf_clear(ringbuf *self) {
    if (self == NULL) return;
    self->head = 0;
    self->count = 0;
}
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd /Users/man.nk/git/clibs/common && make ringbuf`
Expected: `7 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Commit**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/ringbuf
git commit -m "feat(common/ringbuf): malloc-free generic FIFO over caller storage + tests"
```

---

## Task 4: `dll` — doubly linked list over a node pool — TDD

**Files:**
- Create: `common/dll/dll.h`, `common/dll/dll.c`
- Test: `common/dll/test_dll.c`

- [ ] **Step 1: Write the header**

Create `common/dll/dll.h`:
```c
#ifndef CLIBS_DLL_H
#define CLIBS_DLL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Doubly linked list of opaque void* values over a caller-provided node pool.
 * No malloc: capacity == pool size; an internal free-list recycles nodes.
 * The list never copies/frees the stored payload — the caller owns it.
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli. */

typedef enum { DLL_OK, DLL_FULL, DLL_EMPTY, DLL_NOT_FOUND, DLL_INVALID } dll_err;

typedef struct dll_node {
    void *value;
    struct dll_node *prev;
    struct dll_node *next;
} dll_node;

typedef struct {
    dll_node *pool;   /* caller-provided array of cap nodes */
    dll_node *head;
    dll_node *tail;
    dll_node *free;   /* free-list head */
    uint16_t  count;
    uint16_t  cap;
} dll_list;

void     dll_init(dll_list *self, dll_node *pool, uint16_t cap);
dll_err  dll_push_head(dll_list *self, void *value);
dll_err  dll_push_tail(dll_list *self, void *value);
dll_err  dll_pop_head(dll_list *self, void **out);  /* out may be NULL */
dll_err  dll_pop_tail(dll_list *self, void **out);  /* out may be NULL */

/* match(stored_value, key) returns 0 when equal (strcmp-style).
 * On hit: returns DLL_OK and (if found != NULL) *found = node.
 * On miss: returns DLL_NOT_FOUND and *found = NULL. */
dll_err  dll_find(const dll_list *self, const void *key,
                  int (*match)(const void *a, const void *b), dll_node **found);

uint16_t dll_count(const dll_list *self);
bool     dll_is_empty(const dll_list *self);
bool     dll_is_full(const dll_list *self);
void     dll_clear(dll_list *self);

#endif /* CLIBS_DLL_H */
```

- [ ] **Step 2: Write the failing test**

Create `common/dll/test_dll.c`:
```c
#include "unity.h"
#include "dll.h"

void setUp(void) {}
void tearDown(void) {}

/* values stored are pointers to these ints */
static int A = 1, B = 2, C = 3;

static int match_int(const void *stored, const void *key) {
    return *(const int *)stored - *(const int *)key;
}

static void test_init_empty(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
    TEST_ASSERT_FALSE(dll_is_full(&l));
    TEST_ASSERT_EQUAL_UINT16(0, dll_count(&l));
}

static void test_push_tail_pop_head_fifo(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &A));
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &B));
    void *out;
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_pop_head(&l, &out));
    TEST_ASSERT_EQUAL_PTR(&A, out);
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_pop_head(&l, &out));
    TEST_ASSERT_EQUAL_PTR(&B, out);
    TEST_ASSERT_EQUAL_INT(DLL_EMPTY, dll_pop_head(&l, &out));
}

static void test_push_head_and_pop_tail(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    dll_push_head(&l, &A);          /* [A]      */
    dll_push_head(&l, &B);          /* [B, A]   */
    void *out;
    dll_pop_tail(&l, &out); TEST_ASSERT_EQUAL_PTR(&A, out);
    dll_pop_tail(&l, &out); TEST_ASSERT_EQUAL_PTR(&B, out);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
}

static void test_full_capacity(void) {
    dll_node pool[2];
    dll_list l;
    dll_init(&l, pool, 2);
    TEST_ASSERT_EQUAL_INT(DLL_OK,   dll_push_tail(&l, &A));
    TEST_ASSERT_EQUAL_INT(DLL_OK,   dll_push_tail(&l, &B));
    TEST_ASSERT_TRUE(dll_is_full(&l));
    TEST_ASSERT_EQUAL_INT(DLL_FULL, dll_push_tail(&l, &C));
}

static void test_pool_reuse_via_free_list(void) {
    dll_node pool[2];
    dll_list l;
    dll_init(&l, pool, 2);
    void *out;
    dll_push_tail(&l, &A);
    dll_pop_head(&l, &out);          /* node returns to free-list */
    dll_push_tail(&l, &B);           /* must succeed by reusing it */
    dll_push_tail(&l, &C);
    TEST_ASSERT_TRUE(dll_is_full(&l));
    dll_pop_head(&l, &out); TEST_ASSERT_EQUAL_PTR(&B, out);
    dll_pop_head(&l, &out); TEST_ASSERT_EQUAL_PTR(&C, out);
}

static void test_find_hit_and_miss(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    dll_push_tail(&l, &A);
    dll_push_tail(&l, &B);
    dll_node *found = NULL;
    int key = 2;
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_find(&l, &key, match_int, &found));
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(&B, found->value);
    int missing = 99;
    TEST_ASSERT_EQUAL_INT(DLL_NOT_FOUND, dll_find(&l, &missing, match_int, &found));
    TEST_ASSERT_NULL(found);
}

static void test_clear(void) {
    dll_node pool[3];
    dll_list l;
    dll_init(&l, pool, 3);
    dll_push_tail(&l, &A);
    dll_push_tail(&l, &B);
    dll_clear(&l);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
    /* after clear all nodes are free again */
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &C));
    TEST_ASSERT_EQUAL_UINT16(1, dll_count(&l));
}

static void test_invalid_args(void) {
    TEST_ASSERT_EQUAL_INT(DLL_INVALID, dll_push_tail(NULL, &A));
    dll_node pool[1];
    dll_list l;
    dll_init(&l, pool, 1);
    TEST_ASSERT_EQUAL_INT(DLL_INVALID, dll_find(&l, &A, NULL, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_empty);
    RUN_TEST(test_push_tail_pop_head_fifo);
    RUN_TEST(test_push_head_and_pop_tail);
    RUN_TEST(test_full_capacity);
    RUN_TEST(test_pool_reuse_via_free_list);
    RUN_TEST(test_find_hit_and_miss);
    RUN_TEST(test_clear);
    RUN_TEST(test_invalid_args);
    return UNITY_END();
}
```

- [ ] **Step 3: Run the test to verify it fails**

Run: `cd /Users/man.nk/git/clibs/common && make dll`
Expected: undefined references to `dll_init`, `dll_push_tail`, etc.

- [ ] **Step 4: Write the implementation**

Create `common/dll/dll.c`:
```c
#include "dll.h"

void dll_init(dll_list *self, dll_node *pool, uint16_t cap) {
    if (self == NULL) return;
    self->pool  = pool;
    self->head  = NULL;
    self->tail  = NULL;
    self->free  = NULL;
    self->count = 0;
    self->cap   = cap;
    for (uint16_t i = 0; i < cap; i++) {   /* push every node onto the free-list */
        pool[i].value = NULL;
        pool[i].prev  = NULL;
        pool[i].next  = self->free;
        self->free    = &pool[i];
    }
}

static dll_node *node_alloc(dll_list *self) {
    dll_node *n = self->free;
    if (n == NULL) return NULL;
    self->free = n->next;
    n->prev = NULL;
    n->next = NULL;
    n->value = NULL;
    return n;
}

static void node_release(dll_list *self, dll_node *n) {
    n->prev  = NULL;
    n->value = NULL;
    n->next  = self->free;
    self->free = n;
}

dll_err dll_push_head(dll_list *self, void *value) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == self->cap) return DLL_FULL;
    dll_node *n = node_alloc(self);
    if (n == NULL) return DLL_FULL;
    n->value = value;
    n->prev  = NULL;
    n->next  = self->head;
    if (self->head != NULL) self->head->prev = n;
    self->head = n;
    if (self->tail == NULL) self->tail = n;
    self->count++;
    return DLL_OK;
}

dll_err dll_push_tail(dll_list *self, void *value) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == self->cap) return DLL_FULL;
    dll_node *n = node_alloc(self);
    if (n == NULL) return DLL_FULL;
    n->value = value;
    n->next  = NULL;
    n->prev  = self->tail;
    if (self->tail != NULL) self->tail->next = n;
    self->tail = n;
    if (self->head == NULL) self->head = n;
    self->count++;
    return DLL_OK;
}

dll_err dll_pop_head(dll_list *self, void **out) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == 0) return DLL_EMPTY;
    dll_node *n = self->head;
    if (out != NULL) *out = n->value;
    self->head = n->next;
    if (self->head != NULL) self->head->prev = NULL;
    else self->tail = NULL;
    node_release(self, n);
    self->count--;
    return DLL_OK;
}

dll_err dll_pop_tail(dll_list *self, void **out) {
    if (self == NULL) return DLL_INVALID;
    if (self->count == 0) return DLL_EMPTY;
    dll_node *n = self->tail;
    if (out != NULL) *out = n->value;
    self->tail = n->prev;
    if (self->tail != NULL) self->tail->next = NULL;
    else self->head = NULL;
    node_release(self, n);
    self->count--;
    return DLL_OK;
}

dll_err dll_find(const dll_list *self, const void *key,
                 int (*match)(const void *, const void *), dll_node **found) {
    if (self == NULL || match == NULL) return DLL_INVALID;
    for (dll_node *n = self->head; n != NULL; n = n->next) {
        if (match(n->value, key) == 0) {
            if (found != NULL) *found = n;
            return DLL_OK;
        }
    }
    if (found != NULL) *found = NULL;
    return DLL_NOT_FOUND;
}

uint16_t dll_count(const dll_list *self)    { return self ? self->count : 0; }
bool     dll_is_empty(const dll_list *self) { return self ? self->count == 0 : true; }
bool     dll_is_full(const dll_list *self)  { return self ? self->count == self->cap : true; }

void dll_clear(dll_list *self) {
    if (self == NULL) return;
    while (self->head != NULL) {
        dll_node *n = self->head;
        self->head = n->next;
        node_release(self, n);
    }
    self->tail  = NULL;
    self->count = 0;
}
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `cd /Users/man.nk/git/clibs/common && make dll`
Expected: `8 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Commit**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/dll
git commit -m "feat(common/dll): malloc-free doubly linked list over node pool + tests

Replaces dlinklist.c: fixes broken isBusy lock, bad include guard, and the
comparator function-pointer UB that segfaulted the old test."
```

---

## Task 5: `log` — light touch (tests + bug fix)

**Files:**
- Modify: `common/log/src/log.c` (fix the non-color `tag` bug; make color toggle build-flag controllable)
- Test: `common/log/test_log.c`

- [ ] **Step 1: Fix the latent bug + make color toggle controllable**

In `common/log/src/log.c`, replace this line near the top:
```c
#define LOG_USE_COLOR
```
with:
```c
#ifndef LOG_DISABLE_COLOR
#define LOG_USE_COLOR
#endif
```

Then in `stdout_callback`, the non-color `#else` branch uses an undefined `tag`. Replace:
```c
	fprintf(
			ev->outfile, "%s: %s %-5s %s:%d: ",
			tag, buf, level_strings[ev->level], ev->file, ev->line);
```
with:
```c
	fprintf(
			ev->outfile, "%s: %s %-5s %s:%d: ",
			ev->tag, buf, level_strings[ev->level], ev->file, ev->line);
```

- [ ] **Step 2: Write the test**

Create `common/log/test_log.c`:
```c
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
```

- [ ] **Step 3: Run the test to verify it passes**

Run: `cd /Users/man.nk/git/clibs/common && make log`
Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 4: Verify the non-color bug fix compiles**

Run: `cd /Users/man.nk/git/clibs/common && cc -std=c11 -Wall -Wextra -DLOG_DISABLE_COLOR -Ilog/src -c log/src/log.c -o /dev/null && echo OK`
Expected: `OK` (would error `'tag' undeclared` before the Step 1 fix).

- [ ] **Step 5: Commit**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/log/src/log.c common/log/test_log.c
git commit -m "fix(common/log): use ev->tag in non-color branch; gate color by LOG_DISABLE_COLOR; add tests"
```

---

## Task 6: Cleanup, README, final gates

**Files:**
- Delete: `common/dlinklist.c/`, `common/ringbuffchar.c/` (old folders + stale Debug ELF)
- Modify: `common/README.md`

- [ ] **Step 1: Remove the old library folders (and their tracked ELF artifacts)**

Run:
```bash
cd /Users/man.nk/git/clibs
git rm -r common/dlinklist.c common/ringbuffchar.c
ls common      # expect: Makefile  README.md  LICENSE  dll  log  ringbuf  str_utils  third_party  (+ .gitignore)
```

- [ ] **Step 2: Run the full test suite**

Run: `cd /Users/man.nk/git/clibs/common && make clean && make test`
Expected: all four suites print `0 Failures` (str_utils 6, ringbuf 7, dll 8, log 4).

- [ ] **Step 3: Run the strict compile gate**

Run: `cd /Users/man.nk/git/clibs/common && make strict`
Expected: `strict: OK` (no warnings; reusable libs are `-Werror`-clean and AVR-header-free).

- [ ] **Step 4: Rewrite `common/README.md`**

Overwrite `common/README.md`:
```markdown
# clibs/common

Portable, **malloc-free** C libraries owned by the clibs workspace, written to run
on microcontrollers (ATmega/AVR, ESP) as well as on a host PC. Each library is a
transparent struct + `module_<verb>(self, …)` methods over caller-provided
storage — deterministic RAM, no heap. The reusable libs depend only on
`<stdint.h>`, `<stdbool.h>`, `<stddef.h>` (+`<string.h>` for `memcpy`), so they
cross-compile for AVR unchanged.

## Libraries

| Library     | Path          | What it is |
|-------------|---------------|------------|
| `dll`       | `dll/`        | Doubly linked list of `void*` over a caller node pool (free-list, no malloc). |
| `ringbuf`   | `ringbuf/`    | Generic fixed-capacity FIFO over a caller array; value-copy semantics; optional overwrite-oldest. |
| `str_utils` | `str_utils/`  | Freestanding string helpers (`str_len`, `str_compare_n`, `str_cpy_n`, `str_reverse`). |
| `log`       | `log/`        | Leveled logger — fork of [rxi/log.c](https://github.com/rxi/log.c) with a per-message `tag`. Hosted by design; on MCU register a callback that writes to UART. |

## Reusing a library

Copy the library's `.c` + `.h` into your project (e.g. add `dll/dll.c` and
`#include "dll.h"`). Provide your own storage:

```c
dll_node pool[16];
dll_list list;
dll_init(&list, pool, 16);
int v = 42;
dll_push_tail(&list, &v);
```

The containers are **not reentrant**; if you share one with an ISR, guard the
calls with `ATOMIC_BLOCK`/`cli`.

## Tests

Host unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored
under `third_party/unity/`).

```sh
make test     # build + run all suites (dll, ringbuf, str_utils, log)
make strict   # warning-clean compile gate (-Werror) for the reusable libs
make clean
```

## License

MIT — see `LICENSE`. `log/` retains its upstream rxi MIT copyright.
```

- [ ] **Step 5: Commit**

Run:
```bash
cd /Users/man.nk/git/clibs
git add common/README.md common/dlinklist.c common/ringbuffchar.c
git commit -m "refactor(common): drop legacy dlinklist.c/ + ringbuffchar.c/, rewrite README"
```

- [ ] **Step 6: (Optional) push**

Ask the user before pushing. If approved:
```bash
cd /Users/man.nk/git/clibs && git push origin main
```

---

## Self-Review notes

- **Spec coverage:** memory model (static/caller-provided) → Tasks 3,4; OOP transparent-struct → all headers; Unity host tests → Tasks 1-5; log light-touch → Task 5; folder rename + drop Debug/ELF → Tasks 1,6; bug fixes → str_compare_n/str_reverse (T2), comparator-UB/isBusy/guard via redesign (T4), log non-color `tag` (T5); no-fake-locking + ISR contract → headers (T3,T4); strict AVR-clean compile → Task 6 `make strict`.
- **Bug #4 (old test `printf` format mismatches):** resolved by discarding the old `dlinklist.c/main.c` test entirely (Task 6 removes the folder); the new `test_dll.c` has no such code.
- **Type consistency:** error enums (`dll_err`, `rb_err`) and signatures match between each header, its `.c`, and its test.
- **Carry-over:** `ringbuf` substring-search feature intentionally dropped (spec §3); `avr-libs` ringBuffer consolidation is a later cycle.
