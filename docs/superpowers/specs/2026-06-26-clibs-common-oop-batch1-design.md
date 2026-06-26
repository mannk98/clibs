# clibs/common batch 1 — OOP-style portable libs: `bitset` + `mempool` + `fixed` + `crc` + `fsm` (design)

Date: 2026-06-26
Location: `clibs/common/` (the portable, malloc-free library layer).

## Context

`clibs/common/` is the portable layer: malloc-free C that runs on a host PC **and** on MCUs
(ATmega/AVR, ESP). Today it holds `dll`, `ringbuf`, `str_utils`, `log`. The user wants to grow it
into a broader toolbox of **classic + practically-useful** libraries usable on both targets. The full
ambition spans four families (data structures, math, encoding/serial-framing, control/time) ≈ 16 libs —
too large for one cycle, so it becomes a **roadmap** and this spec scopes **batch 1**: five independent
libraries, one-or-more per family, all host-Unity-testable.

- **`bitset`** (data structures) — bit array over a caller `uint32_t[]`.
- **`mempool`** (data structures) — fixed-block allocator; the canonical malloc-free building block.
- **`fixed`** (math) — Q16.16 fixed-point, no FPU.
- **`crc`** (encoding) — canonical `crc8_maxim` / `crc16_ccitt` / `crc16_modbus` / `crc32`.
- **`fsm`** (control) — table-driven finite state machine.

## OOP-in-C conventions (confirmed with user)

C has no `class`; this batch standardises the lightweight "struct + method" OOP the codebase already uses
(`dll`, `ringbuf`) and applies it rigorously to every **stateful** lib. No vtables, no opaque handles —
zero overhead, MCU-friendly.

| Rule | Detail |
|------|--------|
| Class = struct | One transparent struct per lib, named after the lib (the "object"). |
| Constructor | `x_init(self, …)` — always called first, sets every field. |
| Destructor | `x_deinit(self)` **only when there is something to release** — none of these 5 own resources (caller owns storage), so none gets one; the convention is reserved for future libs that do. |
| Methods | `x_verb(self, …)` with `self` as the first parameter (the receiver). |
| Const-correctness | non-mutating queries take `const x *self`; mutators take `x *self`. |
| Encapsulation | struct fields are **private by convention** (stay transparent only so the caller can supply storage with no malloc); callers use methods/accessors, never poke fields. |
| No fn-ptrs in struct | no polymorphism/vtables. The only callback (`fsm` action) is an explicit user callback with `ctx`, not a virtual method. |
| NULL-safe | every method tolerates `NULL self` → no-op / safe default (matches existing libs). |

**Two of the five are not objects** (and stay that way):
- **`crc`** — stateless namespaced functions (no `self`).
- **`fixed`** — a *value type* (`fixed_t`, like `int`) with operations on values (no `self`).

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Selection driver | balanced — classic **and** practically useful; avoid over-specialised libs. |
| Batch 1 scope | the five libs above (one+ per family, all independent, all host-testable). |
| OOP level | lightweight struct + method, standardised (table above). No opaque types, no vtables. |
| `fixed` format | Q16.16 (`int32_t`), fixed — not configurable / not Q8.24 (YAGNI). |
| `crc` vs `esp-libs/crc` | add canonical `common/crc`; **leave `esp-libs/crc` untouched this batch**. `common/crc`'s `crc8_maxim`/`crc16_modbus` are byte-for-byte identical, so esp-libs can re-export it in a later cycle. |
| Out of scope | the other ~11 roadmap libs; any change to `esp-libs`/`avr-libs`; the node_core app. |

## Architecture

```
clibs/common/
├── Makefile        # CHANGED: +5 host-test targets (test -> 9 suites) + strict for the 5
├── README.md       # CHANGED: +5 rows in the library table
├── dll/ ringbuf/ str_utils/ log/ third_party/   # unchanged
├── bitset/    bitset.{h,c}    test_bitset.c
├── mempool/   mempool.{h,c}   test_mempool.c
├── fixed/     fixed.{h,c}     test_fixed.c
├── crc/       crc.{h,c}       test_crc.c
└── fsm/       fsm.{h,c}       test_fsm.c
```

All five are freestanding portable C (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`; `crc`/`mempool` also
`<string.h>` for `memcpy`/byte access), self-contained, no malloc, no cross-dependency between the five.
Conventions = clibs standard (`CLIBS_<MODULE>_H` guards, doc comment per public function, NULL/edge
guards, "Not reentrant — guard with ATOMIC_BLOCK/cli if shared with an ISR").

## Module specs

### `bitset` (`CLIBS_BITSET_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef struct {                 /* fields private */
    uint32_t *words;             /* caller storage: BITSET_WORDS(nbits) words */
    size_t    nbits;             /* capacity in bits */
} bitset;
#define BITSET_WORDS(nbits) (((nbits) + 31u) / 32u)   /* size the caller's array */

void   bitset_init(bitset *self, uint32_t *words, size_t nbits); /* zeroes the words */
void   bitset_set(bitset *self, size_t i);
void   bitset_clear(bitset *self, size_t i);
void   bitset_toggle(bitset *self, size_t i);
bool   bitset_test(const bitset *self, size_t i);     /* out-of-range/NULL -> false */
void   bitset_set_all(bitset *self);                  /* sets exactly nbits (masks last partial word) */
void   bitset_clear_all(bitset *self);
size_t bitset_count(const bitset *self);              /* popcount of set bits */
bool   bitset_find_first_set(const bitset *self, size_t *out); /* lowest set idx -> *out; false if none */
```
- `set_all` must mask the final partial word so `bitset_count` returns exactly `nbits` and no bit ≥ `nbits`
  is set. Out-of-range index: mutators no-op, `bitset_test` → false. `find_first_set` writes `*out` only
  when it returns true.
- Tests: 64-bit set (`uint32_t words[BITSET_WORDS(64)]`). `set(0)`,`set(63)` → `test(0)`/`test(63)` true,
  `test(1)` false, `count`→2; `clear(0)`→count 1; `toggle(1)`→true/count 2, `toggle(1)`→false; after
  `set(5)` `find_first_set`→true,`*out`==5; empty `find_first_set`→false; `set_all` on 64→count 64;
  `set_all` on **nbits=10** (1 word) → count 10, `test(10)` false (partial-word mask); `clear_all`→0;
  out-of-range `set(100)` no-op + `test(100)` false; NULL → safe defaults.

### `mempool` (`CLIBS_MEMPOOL_H`, `<stdint.h>`,`<stddef.h>`,`<stdbool.h>`)
```c
typedef struct {                 /* fields private */
    uint8_t *buf;
    size_t   block_size;         /* effective stride, aligned up */
    size_t   block_count;
    size_t   free_count;
    void    *free_list;          /* free-list head; next-ptr stored in each free block */
} mempool;
#define MEMPOOL_ALIGN      (sizeof(void *))
#define MEMPOOL_STRIDE(bs) (((bs) + (MEMPOOL_ALIGN - 1)) / MEMPOOL_ALIGN * MEMPOOL_ALIGN) /* >= sizeof(void*) */
#define MEMPOOL_BYTES(bs, n) (MEMPOOL_STRIDE(bs) * (n))   /* size the caller's storage */

void   mempool_init(mempool *self, void *storage, size_t block_size, size_t block_count);
void  *mempool_alloc(mempool *self);            /* NULL when exhausted (or NULL self) */
void   mempool_free(mempool *self, void *blk);  /* returns block to pool; NULL blk -> no-op */
size_t mempool_free_count(const mempool *self); /* blocks currently free */
size_t mempool_capacity(const mempool *self);   /* total blocks */
```
- `init` rounds `block_size` up to `MEMPOOL_ALIGN` (and to ≥ `sizeof(void*)` so the free-list next-pointer
  fits), threads the free list through all blocks, sets `free_count = block_count`. Returned blocks are
  aligned to `MEMPOOL_ALIGN`. **Caller must align `storage`** to `MEMPOOL_ALIGN` and size it
  `MEMPOOL_BYTES(block_size, block_count)` (e.g. `_Alignas(MEMPOOL_ALIGN) uint8_t storage[...]`).
- `alloc` pops the free-list head; `free` pushes a block back. Pointer-belongs-to-pool validation and
  double-free detection are **out of scope** (documented as caller's responsibility) to keep it small.
- Tests: `_Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 4)]`; `init(.,sizeof(int),4)`;
  `capacity`→4, `free_count`→4; `p1=alloc`→non-NULL, `free_count`→3; allocate 3 more → `free_count`→0;
  next `alloc`→NULL; `*(int*)p1 = 42` (usable, aligned); `free(p1)`→`free_count`→1; `alloc`→non-NULL
  (reuse); successive allocs return distinct pointers; `free(NULL)` no-op; NULL self → `alloc`→NULL,
  `free` no-op, counts 0.

### `fixed` — Q16.16 value type (`CLIBS_FIXED_H`, `<stdint.h>`)
```c
typedef int32_t fixed_t;                 /* Q16.16: 16 integer bits, 16 fractional bits */
#define FIXED_SHIFT 16
#define FIXED_ONE   (1L << FIXED_SHIFT)  /* 65536 == 1.0 */

fixed_t fixed_from_int(int32_t v);       /* v << 16 */
int32_t fixed_to_int(fixed_t v);         /* integer part, truncated toward zero (matches (int) cast) */
fixed_t fixed_from_float(float v);       /* rounded; host/dev convenience */
float   fixed_to_float(fixed_t v);
fixed_t fixed_add(fixed_t a, fixed_t b); /* a + b */
fixed_t fixed_sub(fixed_t a, fixed_t b); /* a - b */
fixed_t fixed_mul(fixed_t a, fixed_t b); /* (int64_t)a*b >> 16 */
fixed_t fixed_div(fixed_t a, fixed_t b); /* ((int64_t)a << 16) / b; b==0 -> 0 (documented) */
fixed_t fixed_sqrt(fixed_t v);           /* isqrt((uint64_t)v << 16); v<=0 -> 0 */
```
- `fixed_mul`/`fixed_div` use an `int64_t` intermediate (overflow-safe within the Q16.16 range).
  `fixed_to_int` truncates toward zero (e.g. `-2.5 -> -2`). `fixed_div` by zero returns 0 (documented as
  caller's bug, kept deterministic). The float helpers pull in soft-float on MCU — they're optional
  convenience; integer code paths never need them.
- Tests: `to_int(from_int(3))`→3; `mul(from_int(3),from_int(4))`==`from_int(12)`; `div(from_int(12),
  from_int(4))`==`from_int(3)`; `from_float(1.5f)`==98304; `to_float(98304)`≈1.5 (TEST_ASSERT_FLOAT_WITHIN);
  `add`/`sub` on `from_int`; `to_int(from_float(-2.5f))`→-2; `div(x,0)`→0; `sqrt(from_int(4))`≈`from_int(2)`
  and `sqrt(from_int(2))`≈1.41421·ONE (within a few LSB); `sqrt(-ONE)`→0.

### `crc` — stateless functions (`CLIBS_CRC_H`, `<stdint.h>`,`<stddef.h>`)
```c
uint8_t  crc8_maxim(const void *data, size_t len);    /* CRC-8/MAXIM   poly 0x8C refl, init 0x00 */
uint16_t crc16_ccitt(const void *data, size_t len);   /* CRC-16/CCITT-FALSE poly 0x1021, init 0xFFFF */
uint16_t crc16_modbus(const void *data, size_t len);  /* CRC-16/MODBUS poly 0xA001 refl, init 0xFFFF */
uint32_t crc32(const void *data, size_t len);         /* CRC-32 (zlib) poly 0xEDB88320 refl, init/xorout 0xFFFFFFFF */
```
- Bytewise (table-free, flash-light). `data==NULL` or `len==0` → the value over zero bytes.
  `crc8_maxim`/`crc16_modbus` are byte-for-byte identical to the current `esp-libs/crc`.
- Tests against catalogue check values for ASCII `"123456789"` (len 9): `crc8_maxim`→`0xA1`;
  `crc16_ccitt`→`0x29B1`; `crc16_modbus`→`0x4B37`; `crc32`→`0xCBF43926`. Empty/NULL: `crc8_maxim`→`0x00`,
  `crc16_ccitt`→`0xFFFF`, `crc16_modbus`→`0xFFFF`, `crc32`→`0x00000000`.

### `fsm` — table-driven state machine (`CLIBS_FSM_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef struct {                         /* one transition row; lives in flash (const) */
    int   from;                          /* current-state id */
    int   event;                         /* event id */
    int   to;                            /* next-state id */
    void (*action)(void *ctx);           /* optional side effect; may be NULL */
} fsm_transition;

typedef struct {                         /* fields private */
    const fsm_transition *table;
    size_t                count;
    int                   state;         /* current state id */
    void                 *ctx;           /* passed to actions */
} fsm;

void fsm_init(fsm *self, const fsm_transition *table, size_t count, int initial, void *ctx);
int  fsm_state(const fsm *self);         /* current state id; -1 if NULL self */
bool fsm_dispatch(fsm *self, int event); /* first row with from==state && event: run action(ctx) then state=to; true */
```
- `dispatch` does a linear scan; **first matching row wins**. On a match it runs `action(ctx)` (if non-NULL)
  **then** sets `state = to`, returns true. No match → returns false, no state change. The `table` is
  `const` so it lives in flash on an MCU.
- Tests: turnstile — `LOCKED=0`/`UNLOCKED=1`, `COIN=0`/`PUSH=1`, table
  `{L,COIN,U,on_unlock} {U,PUSH,L,on_lock} {L,PUSH,L,NULL} {U,COIN,U,NULL}`, `ctx=&counter`.
  `init(.,table,4,LOCKED,&ctr)`; `state`→LOCKED; `dispatch(COIN)`→true, state UNLOCKED, action ran
  (ctr.unlocks==1); `dispatch(PUSH)`→true, state LOCKED, ctr.locks==1; `dispatch(PUSH)` (LOCKED self-loop,
  NULL action)→true, state LOCKED, no counter change; `dispatch(99)` (unknown)→false, state unchanged;
  NULL self → `dispatch`→false, `state`→-1.

## Build infrastructure changes
- **`common/Makefile`:** add `bitset mempool fixed crc fsm` to `.PHONY` and the `test:` aggregate
  (→ **9 suites**), one run target each (pattern: `$(CC) $(CFLAGS) -I$(UNITY) -I<lib> -o $(BUILD)/test_<lib>
  <lib>/test_<lib>.c <lib>/<lib>.c $(UNITY)/unity.c && ./$(BUILD)/test_<lib>`), and a `strict:` `-Werror`
  compile of each `<lib>.c`.
- **`common/README.md`:** add a row per lib to the library table; note `crc`/`fixed` are functional/value
  (not `self`-objects) and that `crc` is the canonical home (esp-libs copy to be de-duped later).

## Testing
- `cd common && make test` → **9 suites** (the 4 existing + the 5 new), all `0 Failures`.
- `cd common && make strict` → `strict: OK` (all 5 `.c` warning-clean under `-Werror`).
- CI (`.github/workflows/ci.yml`) already runs `make -C common test && strict`, so the new suites are
  covered automatically on push.

## Conventions & scope
- Freestanding portable C, no malloc, OOP rules from the table above (struct+method, ctor `x_init`,
  const queries, private fields, NULL-safe, no vtables). `crc` = functions, `fixed` = value type.
- Out of scope: the ~11 remaining roadmap libs (`vec`/`stack`, `pqueue`, `hashmap`, `rng`, `stats`,
  `hex`, `base64`, `cobs`/`slip`, `tlv`, `scheduler`, `softtimer`, `event`); refactoring `esp-libs/crc`
  to re-export `common/crc`; any `avr-libs`/`esp-libs` change; the node_core app.

## Build sequence
Each lib is independent (own dir, own host test) → the five can be built in parallel (a good fit for the
`embedded-sprint` engine), then one integration step wires them into `Makefile`/`README.md` and runs the
full gate.
1. `bitset` — RED test → impl → GREEN.
2. `mempool` — RED test → impl → GREEN.
3. `fixed` — RED test → impl → GREEN.
4. `crc` — RED test → impl → GREEN.
5. `fsm` — RED test → impl → GREEN.
6. Integration: wire all 5 into `common/Makefile` (test→9 suites, strict) + `common/README.md`; run
   `make test` + `make strict`.
7. clibs feature branch → merge → (push on user's call).

## Roadmap (future cycles, out of scope now)
- **Data structures:** `vec`/`stack`, `pqueue` (binary heap), `hashmap` (open-addressing).
- **Math:** `rng` (xorshift), `stats` (running min/max/mean/variance).
- **Encoding/framing:** `hex`, `base64`, `cobs`/`slip`, `tlv`.
- **Control/time:** `scheduler` (cooperative tick), `softtimer`, `event` (pub/sub); + refactor
  `esp-libs/crc` to re-export `common/crc`.
