# clibs/common batch 3 — OOP portable libs: `hashmap` + `stats` + `cobs` + `scheduler` + `event` (design)

Date: 2026-06-27
Location: `clibs/common/` (the portable, malloc-free library layer).

## Context

Third batch of the `common/` toolbox roadmap (batch 1 merged `2be4a30`, batch 2 merged `7ec582a` → 14
suites). Same conventions, same engine (`embedded-sprint`). Five independent, malloc-free,
host-Unity-tested L1 libs, balanced across families and finishing the high-value classics:

- **`hashmap`** (data structures) — open-addressing string→`void*` map over a caller array.
- **`stats`** (math) — streaming min/max/mean/variance (Welford), no heap.
- **`cobs`** (encoding/framing) — Consistent Overhead Byte Stuffing (zero-free serial framing).
- **`scheduler`** (control/time) — cooperative tick-driven task scheduler.
- **`event`** (control/time) — small publish/subscribe bus.

(`stack` from the roadmap is dropped — `vec` already provides LIFO push/pop.)

## OOP-in-C conventions (locked from batch 1–2)

Transparent struct = the object; constructor `x_init(self,…)`; methods `x_verb(self,…)` self-first;
`const x *self` for non-mutating queries; fields private; **no vtables**; every method NULL-`self`-safe;
`CLIBS_<MODULE>_H` guards; freestanding includes; doc comment per public function/struct; ship a
storage-sizing macro for caller-storage containers (batch-2 lesson). `stats` is a small object; `cobs`
is stateless functions; `hashmap`/`scheduler`/`event` are caller-storage objects. **Callbacks are
allowed as explicit user callbacks** (like `fsm`'s `action`) — `scheduler` task fn and `event` handler
fn are callbacks with a `void *ctx`, **not** vtable polymorphism. **No UB:** no left-shift of a
possibly-negative value (batch-1 lesson); UBSan/ASan pass required in review.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch 3 scope | `hashmap`, `stats`, `cobs`, `scheduler`, `event` (balanced, independent, host-testable). |
| `hashmap` | open addressing + **linear probing** + tombstones; `const char*` keys (FNV-1a hash), `void*` values; keys stored by pointer (caller keeps them alive). No comparator fn-ptr. |
| `stats` | Welford online; **`float`** mean/variance (matches MCU soft-float + Unity float asserts); **population** variance (`m2/count`); **no `stats_stddev`** (avoid `<math.h>`; caller sqrts if needed). |
| `cobs` | standard COBS, zero-free output, **no trailing `0x00` delimiter** (caller owns framing). |
| `scheduler` | array-backed (caller storage), cooperative `scheduler_tick()`; task fn-ptr callback with `ctx` (like `fsm`). Independent — does **not** depend on `pqueue`. |
| `event` | array-backed pub/sub; handler fn-ptr callback `(event_id, data, ctx)`; `publish` returns # handlers invoked. |
| Out of scope | remaining roadmap (`hashmap` is the only DS here; `slip`, `tlv`, `softtimer`); esp-libs/crc re-export; node_core. |

## Architecture

```
clibs/common/
├── Makefile        # CHANGED: +5 host-test targets (test -> 19 suites) + strict for the 5
├── README.md       # CHANGED: +5 rows
├── <batch 0/1/2 libs + third_party>   # unchanged
├── hashmap/    hashmap.{h,c}    test_hashmap.c
├── stats/      stats.{h,c}      test_stats.c
├── cobs/       cobs.{h,c}       test_cobs.c
├── scheduler/  scheduler.{h,c}  test_scheduler.c
└── event/      event.{h,c}      test_event.c
```

All five freestanding, no malloc, no cross-dependency. (`hashmap`/`cobs` use `<string.h>` for
`strcmp`/`memcpy`.)

## Module specs

### `hashmap` — open-addressing string map (`CLIBS_HASHMAP_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef struct {                 /* fields private; key NULL = empty, internal tombstone = deleted */
    const char *key;
    void       *value;
} hashmap_entry;
typedef struct {                 /* fields private */
    hashmap_entry *slots;        /* caller storage: cap entries (zeroed by init) */
    size_t         cap;
    size_t         len;          /* live entries */
} hashmap;

void   hashmap_init(hashmap *self, hashmap_entry *storage, size_t cap);
bool   hashmap_put(hashmap *self, const char *key, void *value);       /* insert/update; false if full or NULL key */
bool   hashmap_get(const hashmap *self, const char *key, void **out);  /* out may be NULL (membership test); false if absent */
bool   hashmap_remove(hashmap *self, const char *key);                 /* false if absent */
size_t hashmap_len(const hashmap *self);
size_t hashmap_cap(const hashmap *self);
```
- FNV-1a hash of the key string, linear probing modulo `cap`, tombstones so probe chains survive a
  remove. `put` updates an existing key's value (len unchanged). Keys are stored **by pointer** — the
  caller must keep key strings alive (documented). `put` returns false only when no empty/tombstone slot
  exists after `cap` probes.
- Acceptance: `hashmap_entry slots[8]`, init → `len 0`. `put("foo",&a)`,`put("bar",&b)` → `len 2`;
  `get("foo")`→`&a`, `get("bar")`→`&b`, `get("baz")` false. `put("foo",&c)` (update) → `len 2`,
  `get("foo")`→`&c`. `remove("foo")` → true, `len 1`, `get("foo")` false; `remove("foo")` again → false.
  Insert 6 distinct keys into the 8-slot map → all 6 retrievable (probing works). Fill to a point where
  no slot remains → `put` of a new key returns false. `get(self,"foo",NULL)` returns presence without
  writing. NULL self/key: `hashmap_put(NULL,..)`/`hashmap_put(self,NULL,..)` false, `hashmap_get(NULL,..)`
  false, `hashmap_len(NULL)` 0.

### `stats` — streaming statistics (`CLIBS_STATS_H`, `<stdint.h>`,`<stddef.h>`)
```c
typedef struct {                 /* fields private */
    size_t  count;
    int32_t min;
    int32_t max;
    float   mean;
    float   m2;                  /* sum of squared deviations (Welford) */
} stats;

void    stats_init(stats *self);
void    stats_add(stats *self, int32_t x);
size_t  stats_count(const stats *self);
int32_t stats_min(const stats *self);       /* 0 if count==0 */
int32_t stats_max(const stats *self);       /* 0 if count==0 */
float   stats_mean(const stats *self);      /* 0.0f if count==0 */
float   stats_variance(const stats *self);  /* population variance m2/count; 0.0f if count==0 */
```
- Welford: `count++; d = x-mean; mean += d/count; m2 += d*(x-mean)`. Population variance `= m2/count`.
- Acceptance: init → `count 0`, `min/max 0`, `mean 0`, `variance 0`. add 2,4,6 → `count 3`, `min 2`,
  `max 6`, `mean≈4.0` (FLOAT_WITHIN), `variance≈2.6667`. add single 5 → `count 1`,`min/max 5`,`mean 5`,
  `variance 0`. add 5,1,9,3 → `min 1`,`max 9`,`mean 4.5`. NULL self: `stats_add(NULL,..)` no crash,
  `stats_count(NULL)` 0, `stats_mean(NULL)` 0.0f, `stats_min(NULL)` 0.

### `cobs` — Consistent Overhead Byte Stuffing (`CLIBS_COBS_H`, `<stdint.h>`,`<stddef.h>`)
```c
/* bytes -> zero-free COBS (no trailing 0x00 delimiter). Returns encoded length, or 0 if out too small / NULL / len 0. */
size_t cobs_encode(const void *data, size_t len, uint8_t *out, size_t out_size);
/* COBS -> original. Returns decoded length, or 0 on malformed / out too small / NULL / len 0. */
size_t cobs_decode(const void *cobs, size_t len, uint8_t *out, size_t out_size);
```
- Standard COBS. Output is guaranteed zero-free (so the caller may append a `0x00` frame delimiter
  itself). Worst-case encoded size = `len + len/254 + 1`; `cobs_encode` returns 0 if `out_size` is below
  what it needs. `cobs_decode` rejects a malformed stream (a code byte pointing past the end) → 0.
- Acceptance (canonical vectors): `encode({0x11,0x22,0x00,0x33},4)` → `{0x03,0x11,0x22,0x02,0x33}` (5);
  `encode({0x11,0x22,0x33,0x44},4)` → `{0x05,0x11,0x22,0x33,0x44}` (5); `encode({0x00},1)` →
  `{0x01,0x01}` (2). The encoded output contains no `0x00` byte. `decode({0x03,0x11,0x22,0x02,0x33},5)` →
  `{0x11,0x22,0x00,0x33}` (4). Round-trip: `decode(encode(x))==x` for the above + a 300-byte all-`0xAA`
  buffer (exercises the 254 block boundary). `encode`/`decode` with `out_size` one short → 0; malformed
  `decode({0x05,0x11},2)` (code overruns) → 0; empty (len 0) → 0; NULL → 0.

### `scheduler` — cooperative tick scheduler (`CLIBS_SCHEDULER_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef void (*scheduler_task_fn)(void *ctx);   /* explicit user callback (like fsm action), not a vtable */
typedef struct {                 /* fields private */
    scheduler_task_fn fn;
    void             *ctx;
    uint32_t          period;    /* ticks between runs (>=1) */
    uint32_t          next_due;  /* tick at which it next runs */
    bool              active;
} scheduler_task;
typedef struct {                 /* fields private */
    scheduler_task *tasks;       /* caller storage: cap slots */
    size_t          cap;
    size_t          count;       /* active tasks */
    uint32_t        now;         /* current tick */
} scheduler;

void   scheduler_init(scheduler *self, scheduler_task *storage, size_t cap);
int    scheduler_add(scheduler *self, scheduler_task_fn fn, void *ctx, uint32_t period); /* task id >=0, or -1 if full / bad args */
bool   scheduler_remove(scheduler *self, int task_id);
void   scheduler_tick(scheduler *self);   /* now++; run every active task whose next_due is reached */
size_t scheduler_count(const scheduler *self);
```
- `add` (period `p>=1`) sets `next_due = now + p` and returns the slot index as the task id. `tick`
  increments `now`, then for each active task with `now >= next_due` runs `fn(ctx)` and advances
  `next_due += period` (steady cadence). `remove` deactivates a task id (frees the slot). `period 0` or a
  full table → `add` returns -1.
- Acceptance: `scheduler_task slots[4]`, init → `count 0`. `add(fnA,&ctr,2)` → id 0, `count 1`. With a
  period-2 task, calling `tick()` 6 times runs `fnA` at ticks 2,4,6 → 3 invocations. Add a second
  period-3 task; over 6 ticks A runs 3× (2,4,6), B runs 2× (3,6). `remove(id)` → task no longer runs,
  `count` decremented. `add` beyond `cap` → -1; `add(.,.,0)` → -1. NULL self: `scheduler_add(NULL,..)`
  -1, `scheduler_tick(NULL)` no crash, `scheduler_count(NULL)` 0.

### `event` — publish/subscribe bus (`CLIBS_EVENT_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef void (*event_handler_fn)(int event_id, void *data, void *ctx);  /* explicit callback, not a vtable */
typedef struct {                 /* fields private */
    int              event_id;
    event_handler_fn fn;
    void            *ctx;
    bool             active;
} event_sub;
typedef struct {                 /* fields private */
    event_sub *subs;             /* caller storage: cap slots */
    size_t     cap;
    size_t     count;            /* active subscriptions */
} event_bus;

void   event_init(event_bus *self, event_sub *storage, size_t cap);
int    event_subscribe(event_bus *self, int event_id, event_handler_fn fn, void *ctx); /* sub id >=0, or -1 if full / NULL fn */
bool   event_unsubscribe(event_bus *self, int sub_id);
size_t event_publish(event_bus *self, int event_id, void *data);  /* calls every active sub for event_id; returns # invoked */
size_t event_count(const event_bus *self);
```
- `subscribe` returns the slot index as a sub id. `publish` invokes every active sub whose `event_id`
  matches, passing `(event_id, data, ctx)`, and returns how many it called. Multiple subs per event
  allowed.
- Acceptance: `event_sub slots[4]`, init → `count 0`. subscribe(1,hA,&ctr)→id0; subscribe(1,hB,&ctr)→id1;
  subscribe(2,hC,&ctr)→id2; `count 3`. `publish(1,data)` → returns 2 (hA,hB ran, each got id 1 + data +
  ctx); `publish(2,..)` → 1; `publish(3,..)` → 0 (no subs). `unsubscribe(0)` → `publish(1,..)` → 1 (only
  hB), `count 2`. subscribe beyond cap → -1; `subscribe(.,.,NULL,.)` → -1. NULL self:
  `event_subscribe(NULL,..)` -1, `event_publish(NULL,..)` 0, `event_count(NULL)` 0.

## Build infrastructure changes
- **`common/Makefile`:** add `hashmap stats cobs scheduler event` to `.PHONY` and the `test:` aggregate
  (→ **19 suites**), one run target each (existing pattern), and a `strict:` `-Werror` compile of each
  `<lib>.c`.
- **`common/README.md`:** add a row per lib to the Libraries table (note `cobs` stateless functions;
  `scheduler`/`event` use explicit callbacks; `hashmap`/`scheduler`/`event` are caller-storage objects).

## Testing
- `cd common && make test` → **19 suites** (14 existing + 5 new), all `0 Failures`.
- `cd common && make strict` → `strict: OK`.
- CI already runs `make -C common test && strict` → covered on push.
- **UBSan+ASan pass** in per-story review for all five (probing, COBS index math, callback arrays) —
  batch-1/2 lessons; codec (`cobs`) gets adversarial malformed-structure vectors (batch-2 lesson).

## Conventions & scope
- Freestanding portable C, no malloc, OOP rules locked from batch 1–2. Callbacks (`scheduler`/`event`)
  are explicit user callbacks with `ctx`, not vtables. `stats` = small object, `cobs` = stateless funcs,
  `hashmap`/`scheduler`/`event` = caller-storage objects.
- Out of scope: remaining roadmap libs (`slip`, `tlv`, `softtimer`); esp-libs/crc re-export; node_core.

## Build sequence
Each lib independent → built in parallel by `embedded-sprint` (Tester RED → Dev GREEN → Reviewer+UBSan),
then one integration step wires `Makefile`/`README.md` (→ 19 suites) and runs the full gate.
1. `hashmap` 2. `stats` 3. `cobs` 4. `scheduler` 5. `event` — each RED→GREEN→commit.
6. Integration: wire all 5 into `common/Makefile` (test→19, strict) + `README.md`; `make test` + `make strict`.
7. clibs feature branch `feat/common-oop-batch3` → merge → push (user's call).
