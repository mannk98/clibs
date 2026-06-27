# clibs/common batch 4 — OOP portable libs: `slip` + `tlv` + `softtimer` (design)

Date: 2026-06-27
Location: `clibs/common/` (the portable, malloc-free library layer).

## Context

Fourth and final `common/` roadmap batch (batches 1–3 merged: `2be4a30`, `7ec582a`, `8147b76` → 19
suites + a `make sanitize` UBSan/ASan gate). Same conventions, same engine (`embedded-sprint`). Three
independent, malloc-free, host-Unity-tested L1 libs that close the toolbox:

- **`slip`** (encoding/framing) — SLIP (RFC 1055) serial framing encode/decode.
- **`tlv`** (encoding) — Type-Length-Value writer + reader (1-byte type, 1-byte length).
- **`softtimer`** (control/time) — wrap-safe poll-based countdown timer (one-shot / auto-reload).

Deferred (NOT in this batch): the `esp-libs/crc` → `common/crc` re-export (cross-component; its real gate
is the absent xtensa compile-gate).

## OOP-in-C conventions (locked from batch 1–3)

Transparent struct = object; ctor `x_init(self,…)`; methods `x_verb(self,…)` self-first; `const x *self`
for queries; private fields; **no vtables**; NULL-`self`-safe; `CLIBS_<MODULE>_H` guards; freestanding
includes; doc comment per public function/struct. `slip` is stateless functions; `tlv` exposes two small
objects (`tlv_writer`, `tlv_reader`); `softtimer` is a small object. **No UB** (no left-shift of a
negative; `softtimer` uses wrap-safe signed-difference tick comparison). `make sanitize` (UBSan+ASan)
must pass.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch 4 scope | `slip`, `tlv`, `softtimer` (the 3 remaining; independent, host-testable). |
| `slip` | RFC 1055 constants (END 0xC0, ESC 0xDB, ESC_END 0xDC, ESC_ESC 0xDD); `slip_encode` emits the escaped payload **plus a trailing END**; `slip_decode` unescapes up to the first unescaped END. |
| `tlv` | 1-byte type + 1-byte length (value ≤ 255 bytes); `tlv_writer` (append) + `tlv_reader` (iterate / find). |
| `softtimer` | single poll-based countdown (`update(elapsed)` advances internal time); one-shot or auto-reload; **wrap-safe** via `(int32_t)(now-expiry) >= 0`. No callback (poll the return value). |
| Out of scope | `esp-libs/crc` re-export; node_core. |

## Architecture

```
clibs/common/
├── Makefile        # CHANGED: +3 host-test targets (test -> 22 suites) + strict + sanitize for the 3
├── README.md       # CHANGED: +3 rows
├── <batch 0/1/2/3 libs + third_party>   # unchanged
├── slip/       slip.{h,c}       test_slip.c
├── tlv/        tlv.{h,c}        test_tlv.c
└── softtimer/  softtimer.{h,c}  test_softtimer.c
```

All three freestanding, no malloc, no cross-dependency. (`slip`/`tlv` use `<string.h>` for `memcpy`.)

## Module specs

### `slip` — SLIP serial framing (`CLIBS_SLIP_H`, `<stdint.h>`,`<stddef.h>`)
```c
/* SLIP (RFC 1055): END=0xC0, ESC=0xDB, ESC_END=0xDC, ESC_ESC=0xDD.
 * bytes -> escaped payload + a trailing END byte. Returns encoded length, or 0 if out too small / NULL / len 0. */
size_t slip_encode(const void *data, size_t len, uint8_t *out, size_t out_size);
/* SLIP frame -> original. Unescapes up to (and consuming) the first unescaped END.
 * Returns decoded payload length, or 0 on malformed (bad escape / no END found) / out too small / NULL / len 0. */
size_t slip_decode(const void *slip, size_t len, uint8_t *out, size_t out_size);
```
- Encode: each `END` byte in the payload → `ESC ESC_END`; each `ESC` → `ESC ESC_ESC`; other bytes
  verbatim; then append one `END`. Worst-case encoded size = `2*len + 1`. Decode: walk input; `ESC`
  followed by `ESC_END`→`END`, by `ESC_ESC`→`ESC`, by anything else → malformed (0); an unescaped `END`
  terminates the frame; reaching `len` with no terminating `END` → malformed (0).
- Acceptance: `encode({0x01,0xC0,0x02},3)` → `{0x01,0xDB,0xDC,0x02,0xC0}` (5);
  `encode({0x01,0xDB,0x02},3)` → `{0x01,0xDB,0xDD,0x02,0xC0}` (5); `encode({0x01,0x02},2)` →
  `{0x01,0x02,0xC0}` (3). `decode({0x01,0xDB,0xDC,0x02,0xC0},5)` → `{0x01,0xC0,0x02}` (3). Round-trip
  `decode(encode(x))==x` for the above + a buffer containing several END/ESC bytes. `decode` stops at the
  first END: `decode({0x01,0xC0,0x99},3)` → `{0x01}` (1). Malformed: `decode({0x01,0xDB,0x99},3)` → 0
  (bad escape); `decode({0x01,0x02},2)` → 0 (no terminating END). `encode`/`decode` with `out_size` one
  short → 0; empty (len 0) → 0; NULL → 0.

### `tlv` — Type-Length-Value writer + reader (`CLIBS_TLV_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`)
```c
typedef struct {                 /* fields private */
    uint8_t *buf; size_t cap; size_t len;
} tlv_writer;
void   tlv_writer_init(tlv_writer *self, void *buf, size_t cap);
bool   tlv_write(tlv_writer *self, uint8_t type, const void *value, uint8_t vlen); /* append [type][vlen][value]; false if no room */
size_t tlv_writer_len(const tlv_writer *self);

typedef struct {                 /* fields private */
    const uint8_t *buf; size_t len; size_t pos;
} tlv_reader;
void   tlv_reader_init(tlv_reader *self, const void *buf, size_t len);
bool   tlv_next(tlv_reader *self, uint8_t *type, const uint8_t **value, uint8_t *vlen); /* advance; false at end / truncated */
bool   tlv_find(const tlv_reader *self, uint8_t type, const uint8_t **value, uint8_t *vlen); /* first record of type, scanning from start; false if absent */
```
- Record layout: `[type:1][vlen:1][value:vlen]`. `tlv_write` needs `2+vlen` free bytes (`false` otherwise).
  `tlv_next` reads the record at `pos` (needs `pos+2 ≤ len` and `pos+2+vlen ≤ len`, else false=truncated),
  sets out params, advances `pos`. `tlv_find` scans independently from the start (does not disturb the
  reader's `pos`; takes `const`). Out params may be NULL? — no: `tlv_next`/`tlv_find` require non-NULL
  out params (documented); NULL self is safe.
- Acceptance: `tlv_writer` over `buf[64]`, init → `len 0`. `tlv_write(1,"ab",2)` → true, `len 4`;
  `tlv_write(2,&x,1)` → true, `len 7`. With a tiny buffer, `tlv_write` of a record that doesn't fit →
  false. `tlv_reader` over the built buffer: `tlv_next` → type 1, vlen 2, value bytes `'a','b'`; `tlv_next`
  → type 2, vlen 1; `tlv_next` → false (end). `tlv_find(2)` → value/vlen of type 2; `tlv_find(9)` → false.
  Malformed: reader over `{0x01,0x05,0xAA}` (claims vlen 5, only 1 byte present) → `tlv_next` false.
  Empty: writer `len 0`; reader `tlv_next` false. NULL self: `tlv_write(NULL,..)` false,
  `tlv_next(NULL,..)` false, `tlv_writer_len(NULL)` 0.

### `softtimer` — wrap-safe countdown timer (`CLIBS_SOFTTIMER_H`, `<stdint.h>`,`<stdbool.h>`)
```c
typedef struct {                 /* fields private */
    uint32_t now;
    uint32_t expiry;
    uint32_t period;             /* 0 = one-shot */
    bool     running;
} softtimer;

void     softtimer_init(softtimer *self);
void     softtimer_start(softtimer *self, uint32_t duration, bool repeating); /* relative to current now; duration>=1 */
bool     softtimer_update(softtimer *self, uint32_t elapsed); /* now += elapsed; returns true if it fired this update */
bool     softtimer_running(const softtimer *self);
void     softtimer_stop(softtimer *self);
uint32_t softtimer_remaining(const softtimer *self);  /* ticks until expiry; 0 if not running or already due */
```
- `start`: `expiry = now + duration`; `period = repeating ? duration : 0`; `running = true`. `update`:
  `now += elapsed`; if `running` and `(int32_t)(now - expiry) >= 0` → fired; if `period != 0`
  `expiry += period` (auto-reload, one fire per update), else `running = false` (one-shot). Returns whether
  it fired. The signed-difference compare is **wrap-safe** across a `uint32_t` rollover.
- Acceptance: init → `running false`, `remaining 0`. `start(100,false)` → `running true`, `remaining 100`;
  `update(50)` → false, `remaining 50`; `update(50)` → true (fires), one-shot → `running false`;
  `update(10)` → false (stopped). Repeating: `start(100,true)`; `update(100)` → true, still `running`,
  `remaining 100`; `update(100)` → true again. `stop()` → `running false`. `remaining` when stopped → 0.
  **Wrap:** `update(0xFFFFFF00)` (advance now near max), `start(0x200,false)` (expiry wraps),
  `update(0x100)` → false, `update(0x100)` → true (fires correctly across the uint32 wrap). NULL self:
  `softtimer_update(NULL,..)` false, `softtimer_running(NULL)` false, `softtimer_remaining(NULL)` 0,
  `softtimer_start(NULL,..)` no crash.

## Build infrastructure changes
- **`common/Makefile`:** add `slip tlv softtimer` to `.PHONY` and the `test:` aggregate (→ **22 suites**),
  one run target each, and a `strict:` `-Werror` compile of each `<lib>.c`. (`sanitize` needs no change —
  it depends on `test`, so the 3 new suites are covered automatically.)
- **`common/README.md`:** add a row per lib (note `slip` stateless functions; `tlv` = writer+reader
  objects; `softtimer` = small object).

## Testing
- `cd common && make test` → **22 suites** (19 existing + 3 new), all `0 Failures`.
- `cd common && make strict` → `strict: OK`.
- `cd common && make sanitize` → all 22 suites clean under UBSan+ASan (the gate added in batch 3;
  `softtimer` wrap math + `slip`/`tlv` index math are the spots to confirm).

## Conventions & scope
- Freestanding portable C, no malloc, OOP rules locked from batch 1–3. `slip` = stateless funcs, `tlv` =
  writer+reader objects, `softtimer` = small object (wrap-safe, no callback). UBSan/ASan via `make sanitize`.
- Out of scope: `esp-libs/crc` re-export; node_core app.

## Build sequence
Each lib independent → built in parallel by `embedded-sprint` (Tester RED → Dev GREEN → Reviewer+sanitize),
then one integration step wires `Makefile`/`README.md` (→ 22 suites) and runs the full gate set incl.
`make sanitize`.
1. `slip` 2. `tlv` 3. `softtimer` — each RED→GREEN→commit.
4. Integration: wire all 3 into `common/Makefile` (test→22, strict) + `README.md`; `make test` + `strict` +
   `sanitize`.
5. clibs feature branch `feat/common-oop-batch4` → merge → push (user's call). This closes the common/ roadmap.
