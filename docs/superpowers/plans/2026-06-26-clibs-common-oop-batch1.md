# clibs/common batch 1 (OOP portable libs) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add five independent, malloc-free, host-tested portable C libraries to `clibs/common/` — `bitset`, `mempool`, `fixed` (Q16.16), `crc`, `fsm` — following the lightweight struct+method OOP-in-C convention.

**Architecture:** Each lib is a self-contained folder `common/<lib>/{<lib>.c,<lib>.h,test_<lib>.c}` with no cross-dependency, developed test-first with Unity. Stateful libs (`bitset`, `mempool`, `fsm`) are transparent-struct "objects" with `x_init` constructors and `x_verb(self,…)` methods; `crc` is stateless functions and `fixed` is a value type. A final integration task wires all five into `common/Makefile` (test → 9 suites, strict) and `common/README.md`.

**Tech Stack:** C11, host `cc` (Apple clang locally / gcc in CI), Unity (vendored at `common/third_party/unity`), GNU make.

**Spec:** `docs/superpowers/specs/2026-06-26-clibs-common-oop-batch1-design.md`

## Global Constraints

- **Work on branch `feat/common-oop-batch1`** (create it before Task 1: `git checkout -b feat/common-oop-batch1`). Merge/push is the user's call after the plan completes.
- **All commands run from `clibs/common/`** unless stated otherwise.
- **Freestanding & portable:** reusable code includes only `<stdint.h>` / `<stdbool.h>` / `<stddef.h>` (+ `<string.h>` where noted). No malloc, no SDK/OS headers. Must compile for AVR/ESP unchanged.
- **OOP-in-C rules:** one transparent struct per stateful lib named after the lib; constructor `x_init(self, …)`; methods `x_verb(self, …)` with `self` first; non-mutating queries take `const x *self`; struct fields are private (callers use methods); no function pointers in structs (the `fsm` action callback is the only callback, by design); every method is NULL-`self`-safe (no-op / safe default). `crc` = stateless functions, `fixed` = value type (no `self`).
- **Header guards:** `CLIBS_<MODULE>_H` (e.g. `CLIBS_BITSET_H`).
- **Compile flags:** host build `-std=c11 -Wall -Wextra -g`; strict gate `-std=c11 -Wall -Wextra -Werror`. Reusable `.c` must be warning-clean under the strict gate.
- **Doc comment** on every public function/struct; mark private fields and "Not reentrant — guard with ATOMIC_BLOCK/cli if shared with an ISR".

---

### Task 1: `bitset` — bit array over caller `uint32_t[]`

**Files:**
- Create: `common/bitset/bitset.h`
- Create: `common/bitset/bitset.c`
- Test: `common/bitset/test_bitset.c`

**Interfaces:**
- Consumes: nothing (independent).
- Produces: `bitset` struct; `BITSET_WORDS(nbits)`; `bitset_init(bitset*, uint32_t*, size_t)`, `bitset_set/clear/toggle(bitset*, size_t)`, `bitset_test(const bitset*, size_t)->bool`, `bitset_set_all/clear_all(bitset*)`, `bitset_count(const bitset*)->size_t`, `bitset_find_first_set(const bitset*, size_t*)->bool`.

- [ ] **Step 1: Write the failing test** — `common/bitset/test_bitset.c`

```c
#include "unity.h"
#include "bitset.h"

void setUp(void) {}
void tearDown(void) {}

static void test_set_test_clear(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    TEST_ASSERT_EQUAL_UINT(0, bitset_count(&bs));
    bitset_set(&bs, 0);
    bitset_set(&bs, 63);
    TEST_ASSERT_TRUE(bitset_test(&bs, 0));
    TEST_ASSERT_TRUE(bitset_test(&bs, 63));
    TEST_ASSERT_FALSE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_UINT(2, bitset_count(&bs));
    bitset_clear(&bs, 0);
    TEST_ASSERT_FALSE(bitset_test(&bs, 0));
    TEST_ASSERT_EQUAL_UINT(1, bitset_count(&bs));
}

static void test_toggle(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs; bitset_init(&bs, words, 64);
    bitset_set(&bs, 63);
    bitset_toggle(&bs, 1);
    TEST_ASSERT_TRUE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_UINT(2, bitset_count(&bs));
    bitset_toggle(&bs, 1);
    TEST_ASSERT_FALSE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_UINT(1, bitset_count(&bs));
}

static void test_find_first_set(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs; bitset_init(&bs, words, 64);
    size_t idx = 999;
    TEST_ASSERT_FALSE(bitset_find_first_set(&bs, &idx));
    bitset_set(&bs, 5);
    bitset_set(&bs, 40);
    TEST_ASSERT_TRUE(bitset_find_first_set(&bs, &idx));
    TEST_ASSERT_EQUAL_UINT(5, idx);
}

static void test_set_all_full_word(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs; bitset_init(&bs, words, 64);
    bitset_set_all(&bs);
    TEST_ASSERT_EQUAL_UINT(64, bitset_count(&bs));
    bitset_clear_all(&bs);
    TEST_ASSERT_EQUAL_UINT(0, bitset_count(&bs));
}

static void test_set_all_partial_word(void) {
    uint32_t words[BITSET_WORDS(10)];
    bitset bs; bitset_init(&bs, words, 10);
    bitset_set_all(&bs);
    TEST_ASSERT_EQUAL_UINT(10, bitset_count(&bs));
    TEST_ASSERT_TRUE(bitset_test(&bs, 9));
    TEST_ASSERT_FALSE(bitset_test(&bs, 10));
}

static void test_out_of_range_and_null(void) {
    uint32_t words[BITSET_WORDS(8)];
    bitset bs; bitset_init(&bs, words, 8);
    bitset_set(&bs, 100);                 /* no-op */
    TEST_ASSERT_FALSE(bitset_test(&bs, 100));
    TEST_ASSERT_EQUAL_UINT(0, bitset_count(&bs));
    bitset_set(NULL, 0);                  /* no crash */
    TEST_ASSERT_FALSE(bitset_test(NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, bitset_count(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_test_clear);
    RUN_TEST(test_toggle);
    RUN_TEST(test_find_first_set);
    RUN_TEST(test_set_all_full_word);
    RUN_TEST(test_set_all_partial_word);
    RUN_TEST(test_out_of_range_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `common/bitset/bitset.h`

```c
#ifndef CLIBS_BITSET_H
#define CLIBS_BITSET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Fixed-capacity bit array over caller-provided uint32_t storage. No malloc.
 * Fields are private — use the methods. Not reentrant: guard with
 * ATOMIC_BLOCK/cli if shared with an ISR. */
typedef struct {
    uint32_t *words;   /* private: caller storage, BITSET_WORDS(nbits) words */
    size_t    nbits;   /* private: capacity in bits */
} bitset;

/* uint32_t words needed to hold `nbits` bits (use to size the caller's array). */
#define BITSET_WORDS(nbits) (((nbits) + 31u) / 32u)

void   bitset_init(bitset *self, uint32_t *words, size_t nbits); /* zeroes the words */
void   bitset_set(bitset *self, size_t i);
void   bitset_clear(bitset *self, size_t i);
void   bitset_toggle(bitset *self, size_t i);
bool   bitset_test(const bitset *self, size_t i);     /* out-of-range/NULL -> false */
void   bitset_set_all(bitset *self);                  /* sets exactly nbits bits */
void   bitset_clear_all(bitset *self);
size_t bitset_count(const bitset *self);              /* number of set bits */
bool   bitset_find_first_set(const bitset *self, size_t *out); /* lowest set idx; false if none */

#endif /* CLIBS_BITSET_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ibitset -o build/test_bitset bitset/test_bitset.c third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols (e.g. `_bitset_init`, `_bitset_count`). (No `bitset.c` yet.)

- [ ] **Step 4: Write the implementation** — `common/bitset/bitset.c`

```c
#include "bitset.h"

void bitset_init(bitset *self, uint32_t *words, size_t nbits) {
    if (!self) return;
    self->words = words;
    self->nbits = nbits;
    bitset_clear_all(self);
}

void bitset_set(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] |= (uint32_t)1u << (i & 31u);
}

void bitset_clear(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] &= ~((uint32_t)1u << (i & 31u));
}

void bitset_toggle(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] ^= (uint32_t)1u << (i & 31u);
}

bool bitset_test(const bitset *self, size_t i) {
    if (!self || i >= self->nbits) return false;
    return ((self->words[i >> 5] >> (i & 31u)) & 1u) != 0u;
}

void bitset_clear_all(bitset *self) {
    if (!self) return;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) self->words[w] = 0u;
}

void bitset_set_all(bitset *self) {
    if (!self || self->nbits == 0) return;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) self->words[w] = 0xFFFFFFFFu;
    size_t rem = self->nbits & 31u;                 /* mask the last partial word */
    if (rem) self->words[nwords - 1] = ((uint32_t)1u << rem) - 1u;
}

size_t bitset_count(const bitset *self) {
    if (!self) return 0;
    size_t nwords = BITSET_WORDS(self->nbits), total = 0;
    for (size_t w = 0; w < nwords; w++) {
        uint32_t x = self->words[w];
        while (x) { x &= x - 1u; total++; }         /* Kernighan popcount */
    }
    return total;
}

bool bitset_find_first_set(const bitset *self, size_t *out) {
    if (!self) return false;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) {
        uint32_t x = self->words[w];
        if (x) {
            size_t bit = 0;
            while ((x & 1u) == 0u) { x >>= 1; bit++; }
            size_t idx = w * 32u + bit;
            if (idx >= self->nbits) return false;
            if (out) *out = idx;
            return true;
        }
    }
    return false;
}
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ibitset -o build/test_bitset bitset/test_bitset.c bitset/bitset.c third_party/unity/unity.c && ./build/test_bitset`
Expected: PASS — `6 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Commit**

```bash
git add common/bitset/
git commit -m "feat(common): add bitset (bit array over caller storage, host-tested)"
```

---

### Task 2: `mempool` — fixed-block allocator

**Files:**
- Create: `common/mempool/mempool.h`
- Create: `common/mempool/mempool.c`
- Test: `common/mempool/test_mempool.c`

**Interfaces:**
- Consumes: nothing (independent).
- Produces: `mempool` struct; `MEMPOOL_ALIGN`, `MEMPOOL_STRIDE(bs)`, `MEMPOOL_BYTES(bs,n)`; `mempool_init(mempool*, void*, size_t, size_t)`, `mempool_alloc(mempool*)->void*`, `mempool_free(mempool*, void*)`, `mempool_free_count(const mempool*)->size_t`, `mempool_capacity(const mempool*)->size_t`.

- [ ] **Step 1: Write the failing test** — `common/mempool/test_mempool.c`

```c
#include "unity.h"
#include "mempool.h"

void setUp(void) {}
void tearDown(void) {}

static void test_capacity_and_exhaustion(void) {
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 4)];
    mempool mp;
    mempool_init(&mp, storage, sizeof(int), 4);
    TEST_ASSERT_EQUAL_UINT(4, mempool_capacity(&mp));
    TEST_ASSERT_EQUAL_UINT(4, mempool_free_count(&mp));
    void *p[4];
    for (int i = 0; i < 4; i++) {
        p[i] = mempool_alloc(&mp);
        TEST_ASSERT_NOT_NULL(p[i]);
    }
    TEST_ASSERT_EQUAL_UINT(0, mempool_free_count(&mp));
    TEST_ASSERT_NULL(mempool_alloc(&mp));      /* exhausted */
}

static void test_distinct_and_usable(void) {
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 3)];
    mempool mp; mempool_init(&mp, storage, sizeof(int), 3);
    int *a = mempool_alloc(&mp);
    int *b = mempool_alloc(&mp);
    TEST_ASSERT_NOT_NULL(a); TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(a != b);
    *a = 42; *b = -7;                          /* aligned + usable */
    TEST_ASSERT_EQUAL_INT(42, *a);
    TEST_ASSERT_EQUAL_INT(-7, *b);
}

static void test_free_returns_block(void) {
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 2)];
    mempool mp; mempool_init(&mp, storage, sizeof(int), 2);
    void *a = mempool_alloc(&mp);
    void *b = mempool_alloc(&mp);
    TEST_ASSERT_EQUAL_UINT(0, mempool_free_count(&mp));
    mempool_free(&mp, a);
    TEST_ASSERT_EQUAL_UINT(1, mempool_free_count(&mp));
    void *c = mempool_alloc(&mp);
    TEST_ASSERT_NOT_NULL(c);                   /* reused */
    TEST_ASSERT_EQUAL_UINT(0, mempool_free_count(&mp));
    (void)b;
}

static void test_null_safe(void) {
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 1)];
    mempool mp; mempool_init(&mp, storage, sizeof(int), 1);
    mempool_free(&mp, NULL);                    /* no-op */
    TEST_ASSERT_EQUAL_UINT(1, mempool_free_count(&mp));
    TEST_ASSERT_NULL(mempool_alloc(NULL));
    mempool_free(NULL, storage);                /* no crash */
    TEST_ASSERT_EQUAL_UINT(0, mempool_capacity(NULL));
    TEST_ASSERT_EQUAL_UINT(0, mempool_free_count(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_capacity_and_exhaustion);
    RUN_TEST(test_distinct_and_usable);
    RUN_TEST(test_free_returns_block);
    RUN_TEST(test_null_safe);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `common/mempool/mempool.h`

```c
#ifndef CLIBS_MEMPOOL_H
#define CLIBS_MEMPOOL_H

#include <stdint.h>
#include <stddef.h>

/* Fixed-block allocator over caller-provided storage. No heap. A free-list
 * threads through the free blocks. Fields are private — use the methods.
 * Not reentrant: guard with ATOMIC_BLOCK/cli if shared with an ISR. */
typedef struct {
    uint8_t *buf;          /* private */
    size_t   block_size;   /* private: effective stride (aligned up) */
    size_t   block_count;  /* private */
    size_t   free_count;   /* private */
    void    *free_list;    /* private: free-list head */
} mempool;

#define MEMPOOL_ALIGN      (sizeof(void *))
/* Per-block stride: block_size rounded up to MEMPOOL_ALIGN (>= sizeof(void*)). */
#define MEMPOOL_STRIDE(bs) ((((bs) + (MEMPOOL_ALIGN - 1)) / MEMPOOL_ALIGN) * MEMPOOL_ALIGN)
/* Bytes the caller's storage must provide for `n` blocks of `bs` bytes each. */
#define MEMPOOL_BYTES(bs, n) (MEMPOOL_STRIDE(bs) * (n))

/* storage must be MEMPOOL_ALIGN-aligned and >= MEMPOOL_BYTES(block_size, block_count). */
void   mempool_init(mempool *self, void *storage, size_t block_size, size_t block_count);
void  *mempool_alloc(mempool *self);            /* one block; NULL when exhausted/NULL self */
void   mempool_free(mempool *self, void *blk);  /* return block to pool; NULL blk -> no-op */
size_t mempool_free_count(const mempool *self); /* blocks currently free */
size_t mempool_capacity(const mempool *self);   /* total blocks */

#endif /* CLIBS_MEMPOOL_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Imempool -o build/test_mempool mempool/test_mempool.c third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols (e.g. `_mempool_init`, `_mempool_alloc`).

- [ ] **Step 4: Write the implementation** — `common/mempool/mempool.c`

```c
#include "mempool.h"

void mempool_init(mempool *self, void *storage, size_t block_size, size_t block_count) {
    if (!self) return;
    size_t stride = MEMPOOL_STRIDE(block_size);
    self->buf = (uint8_t *)storage;
    self->block_size = stride;
    self->block_count = block_count;
    self->free_count = block_count;
    self->free_list = NULL;
    /* Push every block onto the free list (next-ptr stored in the block). */
    for (size_t i = 0; i < block_count; i++) {
        void *blk = self->buf + i * stride;
        *(void **)blk = self->free_list;
        self->free_list = blk;
    }
}

void *mempool_alloc(mempool *self) {
    if (!self || !self->free_list) return NULL;
    void *blk = self->free_list;
    self->free_list = *(void **)blk;
    self->free_count--;
    return blk;
}

void mempool_free(mempool *self, void *blk) {
    if (!self || !blk) return;
    *(void **)blk = self->free_list;
    self->free_list = blk;
    self->free_count++;
}

size_t mempool_free_count(const mempool *self) { return self ? self->free_count : 0; }
size_t mempool_capacity(const mempool *self)   { return self ? self->block_count : 0; }
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Imempool -o build/test_mempool mempool/test_mempool.c mempool/mempool.c third_party/unity/unity.c && ./build/test_mempool`
Expected: PASS — `4 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Commit**

```bash
git add common/mempool/
git commit -m "feat(common): add mempool (fixed-block allocator, host-tested)"
```

---

### Task 3: `fixed` — Q16.16 fixed-point value type

**Files:**
- Create: `common/fixed/fixed.h`
- Create: `common/fixed/fixed.c`
- Test: `common/fixed/test_fixed.c`

**Interfaces:**
- Consumes: nothing (independent).
- Produces: `fixed_t` (`int32_t`), `FIXED_SHIFT`, `FIXED_ONE`; `fixed_from_int/to_int`, `fixed_from_float/to_float`, `fixed_add/sub/mul/div`, `fixed_sqrt`.

- [ ] **Step 1: Write the failing test** — `common/fixed/test_fixed.c`

```c
#include "unity.h"
#include "fixed.h"

void setUp(void) {}
void tearDown(void) {}

static void test_int_roundtrip(void) {
    TEST_ASSERT_EQUAL_INT32(3 * FIXED_ONE, fixed_from_int(3));
    TEST_ASSERT_EQUAL_INT32(3, fixed_to_int(fixed_from_int(3)));
    TEST_ASSERT_EQUAL_INT32(-3, fixed_to_int(fixed_from_int(-3)));
}

static void test_to_int_truncates_toward_zero(void) {
    TEST_ASSERT_EQUAL_INT32(2, fixed_to_int(fixed_from_float(2.9f)));
    TEST_ASSERT_EQUAL_INT32(-2, fixed_to_int(fixed_from_float(-2.5f)));
}

static void test_add_sub(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(7), fixed_add(fixed_from_int(3), fixed_from_int(4)));
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(-1), fixed_sub(fixed_from_int(3), fixed_from_int(4)));
}

static void test_mul_div(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(12), fixed_mul(fixed_from_int(3), fixed_from_int(4)));
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(3), fixed_div(fixed_from_int(12), fixed_from_int(4)));
    TEST_ASSERT_EQUAL_INT32(0, fixed_div(fixed_from_int(5), 0));   /* div by zero -> 0 */
}

static void test_float_convert(void) {
    TEST_ASSERT_EQUAL_INT32(98304, fixed_from_float(1.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, fixed_to_float(98304));
}

static void test_sqrt(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(2), fixed_sqrt(fixed_from_int(4)));
    TEST_ASSERT_INT32_WITHIN(2, fixed_from_float(1.4142135f), fixed_sqrt(fixed_from_int(2)));
    TEST_ASSERT_EQUAL_INT32(0, fixed_sqrt(fixed_from_int(-1)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_int_roundtrip);
    RUN_TEST(test_to_int_truncates_toward_zero);
    RUN_TEST(test_add_sub);
    RUN_TEST(test_mul_div);
    RUN_TEST(test_float_convert);
    RUN_TEST(test_sqrt);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `common/fixed/fixed.h`

```c
#ifndef CLIBS_FIXED_H
#define CLIBS_FIXED_H

#include <stdint.h>

/* Q16.16 signed fixed-point: 16 integer bits + 16 fractional bits. A value
 * type (like int) — no `self` object. mul/div use an int64 intermediate so
 * they don't overflow within the Q16.16 range. The float helpers are host/dev
 * convenience (they pull in soft-float on an MCU). */
typedef int32_t fixed_t;
#define FIXED_SHIFT 16
#define FIXED_ONE   ((fixed_t)(1L << FIXED_SHIFT))   /* 65536 == 1.0 */

fixed_t fixed_from_int(int32_t v);
int32_t fixed_to_int(fixed_t v);          /* integer part, truncated toward zero */
fixed_t fixed_from_float(float v);        /* rounded to nearest */
float   fixed_to_float(fixed_t v);
fixed_t fixed_add(fixed_t a, fixed_t b);
fixed_t fixed_sub(fixed_t a, fixed_t b);
fixed_t fixed_mul(fixed_t a, fixed_t b);
fixed_t fixed_div(fixed_t a, fixed_t b);  /* b == 0 -> 0 */
fixed_t fixed_sqrt(fixed_t v);            /* v <= 0 -> 0 */

#endif /* CLIBS_FIXED_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ifixed -o build/test_fixed fixed/test_fixed.c third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols (e.g. `_fixed_from_int`, `_fixed_mul`).

- [ ] **Step 4: Write the implementation** — `common/fixed/fixed.c`

```c
#include "fixed.h"

fixed_t fixed_from_int(int32_t v) { return (fixed_t)((int64_t)v << FIXED_SHIFT); }

int32_t fixed_to_int(fixed_t v) {
    /* truncate toward zero (matches a C (int) cast) */
    if (v >= 0) return (int32_t)(v >> FIXED_SHIFT);
    return -(int32_t)((-(int64_t)v) >> FIXED_SHIFT);
}

fixed_t fixed_from_float(float v) {
    float scaled = v * (float)FIXED_ONE;
    return (fixed_t)(scaled + (v >= 0.0f ? 0.5f : -0.5f));   /* round to nearest */
}

float fixed_to_float(fixed_t v) { return (float)v / (float)FIXED_ONE; }

fixed_t fixed_add(fixed_t a, fixed_t b) { return a + b; }
fixed_t fixed_sub(fixed_t a, fixed_t b) { return a - b; }

fixed_t fixed_mul(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FIXED_SHIFT);
}

fixed_t fixed_div(fixed_t a, fixed_t b) {
    if (b == 0) return 0;
    return (fixed_t)(((int64_t)a << FIXED_SHIFT) / (int64_t)b);
}

fixed_t fixed_sqrt(fixed_t v) {
    if (v <= 0) return 0;
    uint64_t n = (uint64_t)v << FIXED_SHIFT;     /* result = isqrt(v << 16) */
    uint64_t x = n, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; } /* integer Newton's method */
    return (fixed_t)x;
}
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ifixed -o build/test_fixed fixed/test_fixed.c fixed/fixed.c third_party/unity/unity.c && ./build/test_fixed`
Expected: PASS — `6 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Commit**

```bash
git add common/fixed/
git commit -m "feat(common): add fixed (Q16.16 fixed-point, host-tested)"
```

---

### Task 4: `crc` — canonical checksum functions

**Files:**
- Create: `common/crc/crc.h`
- Create: `common/crc/crc.c`
- Test: `common/crc/test_crc.c`

**Interfaces:**
- Consumes: nothing (independent).
- Produces: `crc8_maxim(const void*, size_t)->uint8_t`, `crc16_ccitt(const void*, size_t)->uint16_t`, `crc16_modbus(const void*, size_t)->uint16_t`, `crc32(const void*, size_t)->uint32_t`.

- [ ] **Step 1: Write the failing test** — `common/crc/test_crc.c`

```c
#include "unity.h"
#include "crc.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static const char *CHECK = "123456789";

static void test_check_values(void) {
    size_t n = strlen(CHECK);
    TEST_ASSERT_EQUAL_HEX8(0xA1, crc8_maxim(CHECK, n));
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt(CHECK, n));
    TEST_ASSERT_EQUAL_HEX16(0x4B37, crc16_modbus(CHECK, n));
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926, crc32(CHECK, n));
}

static void test_empty_and_null(void) {
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim("", 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_ccitt("", 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus("", 0));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, crc32("", 0));
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(NULL, 5));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, crc32(NULL, 5));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_check_values);
    RUN_TEST(test_empty_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `common/crc/crc.h`

```c
#ifndef CLIBS_CRC_H
#define CLIBS_CRC_H

#include <stdint.h>
#include <stddef.h>

/* Canonical checksum functions (stateless, bytewise, table-free).
 * data == NULL or len == 0 returns the checksum over zero bytes. */
uint8_t  crc8_maxim(const void *data, size_t len);    /* CRC-8/MAXIM:   poly 0x8C refl, init 0x00 */
uint16_t crc16_ccitt(const void *data, size_t len);   /* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF */
uint16_t crc16_modbus(const void *data, size_t len);  /* CRC-16/MODBUS: poly 0xA001 refl, init 0xFFFF */
uint32_t crc32(const void *data, size_t len);         /* CRC-32 (zlib): poly 0xEDB88320 refl, init/xorout 0xFFFFFFFF */

#endif /* CLIBS_CRC_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Icrc -o build/test_crc crc/test_crc.c third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols (e.g. `_crc8_maxim`, `_crc32`).

- [ ] **Step 4: Write the implementation** — `common/crc/crc.c`

```c
#include "crc.h"

uint8_t crc8_maxim(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint8_t crc = 0x00;
    for (size_t i = 0; p && i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1u) ? (uint8_t)((crc >> 1) ^ 0x8C) : (uint8_t)(crc >> 1);
    }
    return crc;
}

uint16_t crc16_ccitt(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; p && i < len; i++) {
        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

uint16_t crc16_modbus(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; p && i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
    }
    return crc;
}

uint32_t crc32(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; p && i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
    }
    return crc ^ 0xFFFFFFFFu;
}
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Icrc -o build/test_crc crc/test_crc.c crc/crc.c third_party/unity/unity.c && ./build/test_crc`
Expected: PASS — `2 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Commit**

```bash
git add common/crc/
git commit -m "feat(common): add crc (crc8_maxim/crc16_ccitt/crc16_modbus/crc32, host-tested)"
```

---

### Task 5: `fsm` — table-driven finite state machine

**Files:**
- Create: `common/fsm/fsm.h`
- Create: `common/fsm/fsm.c`
- Test: `common/fsm/test_fsm.c`

**Interfaces:**
- Consumes: nothing (independent).
- Produces: `fsm_transition` struct (`int from, event, to; void (*action)(void*)`), `fsm` struct; `fsm_init(fsm*, const fsm_transition*, size_t, int, void*)`, `fsm_state(const fsm*)->int`, `fsm_dispatch(fsm*, int)->bool`.

- [ ] **Step 1: Write the failing test** — `common/fsm/test_fsm.c`

```c
#include "unity.h"
#include "fsm.h"

void setUp(void) {}
void tearDown(void) {}

enum { LOCKED = 0, UNLOCKED = 1 };
enum { COIN = 0, PUSH = 1 };

typedef struct { int unlocks; int locks; } turnstile_ctx;

static void on_unlock(void *ctx) { ((turnstile_ctx *)ctx)->unlocks++; }
static void on_lock(void *ctx)   { ((turnstile_ctx *)ctx)->locks++; }

static const fsm_transition TABLE[] = {
    { LOCKED,   COIN, UNLOCKED, on_unlock },
    { UNLOCKED, PUSH, LOCKED,   on_lock   },
    { LOCKED,   PUSH, LOCKED,   NULL      },
    { UNLOCKED, COIN, UNLOCKED, NULL      },
};
#define TABLE_N (sizeof(TABLE) / sizeof(TABLE[0]))

static void test_transitions_and_actions(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, TABLE, TABLE_N, LOCKED, &ctx);
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));

    TEST_ASSERT_TRUE(fsm_dispatch(&m, COIN));
    TEST_ASSERT_EQUAL_INT(UNLOCKED, fsm_state(&m));
    TEST_ASSERT_EQUAL_INT(1, ctx.unlocks);

    TEST_ASSERT_TRUE(fsm_dispatch(&m, PUSH));
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));
    TEST_ASSERT_EQUAL_INT(1, ctx.locks);
}

static void test_self_loop_null_action(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m; fsm_init(&m, TABLE, TABLE_N, LOCKED, &ctx);
    TEST_ASSERT_TRUE(fsm_dispatch(&m, PUSH));    /* LOCKED + PUSH -> LOCKED, NULL action */
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));
    TEST_ASSERT_EQUAL_INT(0, ctx.unlocks);
    TEST_ASSERT_EQUAL_INT(0, ctx.locks);
}

static void test_unknown_event_and_null(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m; fsm_init(&m, TABLE, TABLE_N, LOCKED, &ctx);
    TEST_ASSERT_FALSE(fsm_dispatch(&m, 99));     /* no matching row */
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));
    TEST_ASSERT_FALSE(fsm_dispatch(NULL, COIN));
    TEST_ASSERT_EQUAL_INT(-1, fsm_state(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_transitions_and_actions);
    RUN_TEST(test_self_loop_null_action);
    RUN_TEST(test_unknown_event_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `common/fsm/fsm.h`

```c
#ifndef CLIBS_FSM_H
#define CLIBS_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Table-driven finite state machine. The transition table is const (lives in
 * flash on an MCU). Fields are private — use the methods. Not reentrant. */
typedef struct {
    int   from;                 /* current-state id */
    int   event;                /* event id */
    int   to;                   /* next-state id */
    void (*action)(void *ctx);  /* optional side effect; may be NULL */
} fsm_transition;

typedef struct {
    const fsm_transition *table;   /* private */
    size_t                count;   /* private */
    int                   state;   /* private: current state */
    void                 *ctx;     /* private: passed to actions */
} fsm;

void fsm_init(fsm *self, const fsm_transition *table, size_t count, int initial, void *ctx);
int  fsm_state(const fsm *self);          /* current state id; -1 if self is NULL */
bool fsm_dispatch(fsm *self, int event);  /* first row matching from+event: action then state=to; true */

#endif /* CLIBS_FSM_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ifsm -o build/test_fsm fsm/test_fsm.c third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols (e.g. `_fsm_init`, `_fsm_dispatch`).

- [ ] **Step 4: Write the implementation** — `common/fsm/fsm.c`

```c
#include "fsm.h"

void fsm_init(fsm *self, const fsm_transition *table, size_t count, int initial, void *ctx) {
    if (!self) return;
    self->table = table;
    self->count = count;
    self->state = initial;
    self->ctx = ctx;
}

int fsm_state(const fsm *self) { return self ? self->state : -1; }

bool fsm_dispatch(fsm *self, int event) {
    if (!self || !self->table) return false;
    for (size_t i = 0; i < self->count; i++) {
        const fsm_transition *t = &self->table[i];
        if (t->from == self->state && t->event == event) {
            if (t->action) t->action(self->ctx);
            self->state = t->to;
            return true;
        }
    }
    return false;
}
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -Ithird_party/unity -Ifsm -o build/test_fsm fsm/test_fsm.c fsm/fsm.c third_party/unity/unity.c && ./build/test_fsm`
Expected: PASS — `3 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Commit**

```bash
git add common/fsm/
git commit -m "feat(common): add fsm (table-driven state machine, host-tested)"
```

---

### Task 6: Integration — wire the 5 libs into `Makefile` + `README.md` and run the full gate

**Files:**
- Modify: `common/Makefile`
- Modify: `common/README.md`

**Interfaces:**
- Consumes: all 5 libs from Tasks 1–5.
- Produces: `make test` → 9 suites; `make strict` → covers all 5; documented library table.

- [ ] **Step 1: Update `common/Makefile`** — add the 5 libs to `.PHONY`, the `test:` aggregate, one run target each, and the `strict:` gate.

Replace the `.PHONY` + `test:` lines:
```makefile
.PHONY: test str_utils ringbuf dll log bitset mempool fixed crc fsm strict clean
test: str_utils ringbuf dll log bitset mempool fixed crc fsm
```

Add these five targets immediately after the existing `log:` target (before `strict:`):
```makefile
bitset: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibitset -o $(BUILD)/test_bitset \
	    bitset/test_bitset.c bitset/bitset.c $(UNITY)/unity.c
	./$(BUILD)/test_bitset

mempool: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imempool -o $(BUILD)/test_mempool \
	    mempool/test_mempool.c mempool/mempool.c $(UNITY)/unity.c
	./$(BUILD)/test_mempool

fixed: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ifixed -o $(BUILD)/test_fixed \
	    fixed/test_fixed.c fixed/fixed.c $(UNITY)/unity.c
	./$(BUILD)/test_fixed

crc: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Icrc -o $(BUILD)/test_crc \
	    crc/test_crc.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_crc

fsm: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ifsm -o $(BUILD)/test_fsm \
	    fsm/test_fsm.c fsm/fsm.c $(UNITY)/unity.c
	./$(BUILD)/test_fsm
```

Add these five lines to the `strict:` recipe, immediately before the `@echo "strict: OK"` line:
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibitset  -c bitset/bitset.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imempool -c mempool/mempool.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifixed   -c fixed/fixed.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Icrc     -c crc/crc.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifsm     -c fsm/fsm.c         -o /dev/null
```

- [ ] **Step 2: Run the full test gate to verify all 9 suites pass**

Run: `make clean && make test`
Expected: each suite prints `OK`; the new ones report `6 Tests`/`4 Tests`/`6 Tests`/`2 Tests`/`3 Tests`, all `0 Failures`. 9 suites total (str_utils, ringbuf, dll, log, bitset, mempool, fixed, crc, fsm).

- [ ] **Step 3: Run the strict gate**

Run: `make strict`
Expected: `strict: OK` (all reusable `.c`, including the 5 new ones, compile warning-clean under `-Werror`).

- [ ] **Step 4: Update `common/README.md`** — add five rows to the Libraries table (after the `log` row) and a note on `crc`/`fixed` being functional/value types.

Add these rows to the `## Libraries` table:
```markdown
| `bitset`    | `bitset/`     | Bit array over a caller `uint32_t[]` (set/clear/toggle/test/count/find-first-set). |
| `mempool`   | `mempool/`    | Fixed-block allocator (alloc/free fixed-size blocks from a caller buffer) — the malloc-free building block. |
| `fixed`     | `fixed/`      | Q16.16 fixed-point value type (mul/div/sqrt + int/float conversion, no FPU). |
| `crc`       | `crc/`        | Canonical checksums: `crc8_maxim`, `crc16_ccitt`, `crc16_modbus`, `crc32` (stateless functions). |
| `fsm`       | `fsm/`        | Table-driven finite state machine (states × events → action + next state). |
```

Add this note immediately after the table:
```markdown
`crc` (stateless functions) and `fixed` (a value type like `int`) are not
`self`-objects — the OOP struct+method convention applies only to the stateful
libs (`dll`, `ringbuf`, `bitset`, `mempool`, `fsm`). `crc` is the canonical
home for these checksums; `esp-libs/crc` will be de-duped to re-export it later.
```

Also update the `make test` line in the `## Tests` section to read:
```markdown
make test     # build + run all suites (str_utils, ringbuf, dll, log, bitset, mempool, fixed, crc, fsm)
```

- [ ] **Step 5: Re-run the gate after the doc change (sanity) and commit**

Run: `make test && make strict`
Expected: all suites `OK`; `strict: OK`.

```bash
git add common/Makefile common/README.md
git commit -m "build(common): wire bitset/mempool/fixed/crc/fsm into make test+strict and README (9 suites)"
```

---

## Self-Review

**1. Spec coverage** — every spec item maps to a task:
- `bitset`/`mempool`/`fixed`/`crc`/`fsm` module specs → Tasks 1–5 (each with the spec's exact API + test cases incl. edge/NULL).
- OOP conventions (struct+method, `x_init`, const queries, private fields, NULL-safe, no vtables) → applied in every header/impl; `crc`/`fixed` kept functional/value.
- `crc` check values (`0xA1`/`0x29B1`/`0x4B37`/`0xCBF43926`) and empty/NULL behavior → Task 4 tests.
- `fixed` Q16.16, div-by-zero→0, toward-zero truncation, sqrt → Task 3 tests.
- Build infra (`Makefile` test→9 suites + strict; `README` rows; CI already runs these) → Task 6.
- `esp-libs/crc` left untouched; roadmap libs out of scope → no task touches them (correct).

**2. Placeholder scan** — no TBD/TODO/"handle edge cases"/"similar to"; every code step shows complete code; every run step shows the exact command + expected output. Clean.

**3. Type consistency** — names/signatures identical across header, impl, test, and Makefile in every task: `bitset_find_first_set(const bitset*, size_t*)`, `mempool_alloc(mempool*)`, `MEMPOOL_BYTES`, `fixed_from_int`/`fixed_to_int`, `crc16_ccitt`, `fsm_dispatch(fsm*, int)`. `fixed_to_int` toward-zero matches the `-2.5 → -2` test. `fsm_state(NULL) == -1` matches the test. Consistent.
