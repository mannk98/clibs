# clibs/common batch 2 — OOP portable libs: `vec` + `pqueue` + `rng` + `hex` + `base64` (design)

Date: 2026-06-27
Location: `clibs/common/` (the portable, malloc-free library layer).

## Context

Second batch of the `common/` toolbox roadmap (batch 1 — `bitset`/`mempool`/`fixed`/`crc`/`fsm` — merged
`2be4a30`). Same conventions, same engine (`embedded-sprint`). Five independent, malloc-free,
host-Unity-tested L1 libs, one-or-more per family:

- **`vec`** (data structures) — generic dynamic array + stack over a caller buffer.
- **`pqueue`** (data structures) — binary min-heap (priority queue) over a caller array.
- **`rng`** (math) — xorshift32 PRNG.
- **`hex`** (encoding) — hex encode/decode.
- **`base64`** (encoding) — Base64 (RFC 4648) encode/decode.

## OOP-in-C conventions (locked from batch 1)

Transparent struct = the object; constructor `x_init(self,…)`; methods `x_verb(self,…)` with `self`
first; non-mutating queries take `const x *self`; fields private (callers use methods); **no function
pointers in structs / no vtables**; every method NULL-`self`-safe (no-op / safe default);
`CLIBS_<MODULE>_H` guards; freestanding includes (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`, +`<string.h>`
for `memcpy` where needed); doc comment per public function/struct. `rng` is a small stateful object;
`hex`/`base64` are stateless functions (no `self`). **No UB:** never left-shift a possibly-negative
value (batch-1 lesson) — `rng` shifts only `uint32_t`.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch 2 scope | `vec`, `pqueue`, `rng`, `hex`, `base64` (balanced, independent, host-testable). |
| `pqueue` ordering | min-heap keyed by `int32_t` + opaque `void *value` payload — **no comparator fn-ptr** (keeps the no-vtable rule; covers scheduler-by-deadline later). |
| `vec` | one lib doubling as vector (random-access get/set) and stack (push/pop = LIFO). |
| encode/decode return | bytes/chars written; **0 = empty-or-error** (documented); no NUL-termination written. |
| Out of scope | remaining roadmap libs (`stack` standalone, `hashmap`, `stats`, `cobs`/`slip`, `tlv`, `scheduler`, `softtimer`, `event`); esp-libs/crc re-export; node_core. |

## Architecture

```
clibs/common/
├── Makefile        # CHANGED: +5 host-test targets (test -> 14 suites) + strict for the 5
├── README.md       # CHANGED: +5 rows in the library table
├── <batch-0/1 libs: dll, ringbuf, str_utils, log, bitset, mempool, fixed, crc, fsm, third_party>  # unchanged
├── vec/       vec.{h,c}       test_vec.c
├── pqueue/    pqueue.{h,c}    test_pqueue.c
├── rng/       rng.{h,c}       test_rng.c
├── hex/       hex.{h,c}       test_hex.c
└── base64/    base64.{h,c}    test_base64.c
```

All five freestanding, no malloc, no cross-dependency. (`vec`/`hex`/`base64` use `<string.h>` for
`memcpy`.)

## Module specs

### `vec` — generic array + stack (`CLIBS_VEC_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`,`<string.h>`)
```c
typedef struct {                 /* fields private */
    uint8_t *buf;                /* caller storage: cap*elem_size bytes */
    size_t   elem_size;
    size_t   cap;
    size_t   len;
} vec;
#define VEC_BYTES(elem_size, cap) ((elem_size) * (cap))   /* size the caller's buffer */

void   vec_init(vec *self, void *storage, size_t cap, size_t elem_size);
bool   vec_push(vec *self, const void *elem);            /* append (copy elem_size bytes); false if full */
bool   vec_pop(vec *self, void *out);                    /* remove last; out may be NULL; false if empty */
bool   vec_get(const vec *self, size_t i, void *out);    /* copy element i to out; false if i>=len */
bool   vec_set(vec *self, size_t i, const void *elem);   /* overwrite element i; false if i>=len */
size_t vec_len(const vec *self);
size_t vec_cap(const vec *self);
bool   vec_is_empty(const vec *self);
bool   vec_is_full(const vec *self);
void   vec_clear(vec *self);                             /* len = 0 (storage untouched) */
```
- Value semantics (memcpy of `elem_size`). `vec_push`/`vec_pop` give LIFO stack use; `vec_get`/`vec_set`
  give random access.
- Acceptance: `int` storage, cap 4. init → `len 0`, `cap 4`, `is_empty`, `!is_full`. push 1,2,3 → `len 3`;
  `get(0)=1,get(1)=2,get(2)=3`; `get(3)` false. push 4 → `is_full`; push 5 → false, `len 4`. `set(1,99)` →
  `get(1)=99`; `set(9,..)` false. pop → out=4, `len 3`; pop to empty → `is_empty`; pop empty → false (out
  untouched). `clear` → `len 0`. NULL self: `vec_push(NULL,..)` false, `vec_pop(NULL,..)` false,
  `vec_get(NULL,..)` false, `vec_len(NULL)` 0, `vec_is_empty(NULL)` true.

### `pqueue` — binary min-heap (`CLIBS_PQUEUE_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef struct { int32_t key; void *value; } pqueue_node;   /* lower key = higher priority */
typedef struct {                 /* fields private */
    pqueue_node *nodes;          /* caller storage: cap entries */
    size_t       cap;
    size_t       len;
} pqueue;

void   pqueue_init(pqueue *self, pqueue_node *storage, size_t cap);
bool   pqueue_push(pqueue *self, int32_t key, void *value);            /* false if full */
bool   pqueue_pop_min(pqueue *self, int32_t *key_out, void **val_out); /* remove min; outs may be NULL; false if empty */
bool   pqueue_peek_min(const pqueue *self, int32_t *key_out, void **val_out); /* false if empty */
size_t pqueue_len(const pqueue *self);
bool   pqueue_is_empty(const pqueue *self);
```
- Array-backed binary heap: `push` sift-up, `pop_min` swaps root with last + sift-down. Min key is the
  root. `value` is an opaque payload (may be NULL).
- Acceptance: `pqueue_node storage[8]`, init → `len 0`, `is_empty`. push (5,a),(1,b),(3,c) → `len 3`;
  `peek_min` → `key 1`,`val b` (no removal, `len 3`). `pop_min` → 1 then 3 then 5 (ascending, values
  b,c,a); after three pops `is_empty`. Fill to cap then push → false. `pop_min`/`peek_min` on empty →
  false (outs untouched). `pop_min(self,NULL,NULL)` still removes the min. Duplicate keys allowed (both
  pop). NULL self: `pqueue_push(NULL,..)` false, `pqueue_pop_min(NULL,..)` false, `pqueue_len(NULL)` 0,
  `pqueue_is_empty(NULL)` true.

### `rng` — xorshift32 PRNG (`CLIBS_RNG_H`, `<stdint.h>`)
```c
typedef struct { uint32_t state; } rng;   /* private */

void     rng_seed(rng *self, uint32_t seed);    /* seed 0 is coerced to 1 (xorshift can't be 0) */
uint32_t rng_next(rng *self);                   /* next 32-bit value (xorshift32); 0 if NULL self */
uint32_t rng_below(rng *self, uint32_t bound);  /* uniform-ish in [0,bound); bound 0 -> 0 */
```
- xorshift32: `x ^= x<<13; x ^= x>>17; x ^= x<<5` (all on `uint32_t` — defined). Deterministic and
  reproducible from a seed. `rng_below` = `rng_next() % bound` (modulo bias acceptable for a small PRNG;
  documented).
- Acceptance: two `rng` seeded with the same value produce the **identical** `rng_next` sequence
  (reproducible); a single `rng`'s successive `rng_next` values are not all equal. `rng_seed(.,0)` then
  `rng_next` is non-zero and deterministic (state coerced to 1). `rng_below(.,10)` over 1000 draws always
  in `[0,10)`; `rng_below(.,1)==0`; `rng_below(.,0)==0`. NULL self: `rng_next(NULL)==0`,
  `rng_below(NULL,5)==0`, `rng_seed(NULL,1)` no crash.

### `hex` — hex encode/decode (`CLIBS_HEX_H`, `<stdint.h>`,`<stddef.h>`)
```c
/* bytes -> lowercase hex (no NUL). Returns chars written (len*2), or 0 if out_size<len*2 / NULL / len 0. */
size_t hex_encode(const void *data, size_t len, char *out, size_t out_size);
/* hex -> bytes. Returns bytes written (hex_len/2), or 0 on odd length / invalid char / out too small / NULL / 0. */
size_t hex_decode(const char *hex, size_t hex_len, void *out, size_t out_size);
```
- `hex_encode` lowercase `0-9a-f`. `hex_decode` accepts upper+lower; rejects odd `hex_len` and any
  non-hex char (returns 0, writes nothing on error).
- Acceptance: `hex_encode({0xDE,0xAD,0xBE,0xEF},4)` → `"deadbeef"`, returns 8. `out_size` 7 → 0.
  `hex_decode("deadbeef",8)` → `{0xDE,0xAD,0xBE,0xEF}`, returns 4. `hex_decode("DEADBEEF",8)` → same
  (uppercase). `hex_decode("abc",3)` → 0 (odd). `hex_decode("zz",2)` → 0 (invalid). `hex_decode` with
  `out_size` 1 for 8 chars → 0 (too small). Empty (`len`/`hex_len` 0) → 0. NULL data/out → 0 / no crash.

### `base64` — RFC 4648 (`CLIBS_BASE64_H`, `<stdint.h>`,`<stddef.h>`)
```c
/* bytes -> base64 (alphabet A-Za-z0-9+/, '=' padding, no NUL). Returns chars written (4*ceil(len/3)),
 * or 0 if out_size too small / NULL / len 0. */
size_t base64_encode(const void *data, size_t len, char *out, size_t out_size);
/* base64 -> bytes. Returns bytes written, or 0 on bad length (not mult 4) / invalid char / out too small / NULL / 0. */
size_t base64_decode(const char *b64, size_t b64_len, void *out, size_t out_size);
```
- Standard alphabet with `=` padding. `base64_decode` requires `b64_len` a multiple of 4; handles 0/1/2
  `=` pad; rejects invalid chars (returns 0).
- Acceptance (RFC 4648 vectors): `encode("foobar",6)`→`"Zm9vYmFy"` (8); `encode("fo",2)`→`"Zm8="` (4);
  `encode("f",1)`→`"Zg=="` (4). `decode("Zm9vYmFy",8)`→`"foobar"` (6); `decode("Zm8=",4)`→`"fo"` (2);
  `decode("Zg==",4)`→`"f"` (1). `decode("abc",3)` → 0 (not mult 4). `decode("####",4)` → 0 (invalid).
  encode `out_size` too small → 0. Empty → 0. NULL → 0 / no crash.

## Build infrastructure changes
- **`common/Makefile`:** add `vec pqueue rng hex base64` to `.PHONY` and the `test:` aggregate (→ **14
  suites**), one run target each (pattern: `$(CC) $(CFLAGS) -I$(UNITY) -I<lib> -o $(BUILD)/test_<lib>
  <lib>/test_<lib>.c <lib>/<lib>.c $(UNITY)/unity.c && ./$(BUILD)/test_<lib>`), and a `strict:` `-Werror`
  compile of each `<lib>.c`.
- **`common/README.md`:** add a row per lib to the Libraries table (note `hex`/`base64` are stateless
  functions; `rng` is a small object).

## Testing
- `cd common && make test` → **14 suites** (the 9 existing + the 5 new), all `0 Failures`.
- `cd common && make strict` → `strict: OK`.
- CI (`.github/workflows/ci.yml`) already runs `make -C common test && strict` → covered on push.
- **UBSan pass** in per-story review for `rng` (shift-heavy) and the encoders (index/pointer math) — batch-1 lesson.

## Conventions & scope
- Freestanding portable C, no malloc, OOP rules locked from batch 1 (struct+method, ctor `x_init`, const
  queries, private fields, NULL-safe, no vtables). `hex`/`base64` = stateless functions, `rng` = small object.
- Out of scope: remaining roadmap libs; esp-libs/crc re-export; node_core app.

## Build sequence
Each lib independent → built in parallel by `embedded-sprint` (Tester RED → Dev GREEN → Reviewer), then
one integration step wires `Makefile`/`README.md` (→ 14 suites) and runs the full gate.
1. `vec` 2. `pqueue` 3. `rng` 4. `hex` 5. `base64` — each RED→GREEN→commit.
6. Integration: wire all 5 into `common/Makefile` (test→14, strict) + `README.md`; `make test` + `make strict`.
7. clibs feature branch `feat/common-oop-batch2` → merge → push (user's call).
