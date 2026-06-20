# clibs `common/` — MCU-lightweight overhaul (design)

Date: 2026-06-20
Scope: **cycle 1 = `common/` only** (`dll`, `ringbuf`, `str_utils`, `log`).
`avr-libs` (16 register-level modules) and `esp-idf-template` are out of scope — later cycles.

## Goal

Make the clibs-owned portable C libraries correct, tested, and **suitable to run on a
microcontroller** (ATmega / ESP). Priorities, in order:

1. **Lightweight / MCU-correct**: no dynamic allocation, deterministic RAM, freestanding (no host-only headers in the reusable libs).
2. **OOP-in-C**: transparent struct + `self`-methods (`module_<verb>(self, …)`); fields private-by-convention; **no per-instance function pointers** (they waste RAM and aren't needed here).
3. **Tested**: host-run unit tests with Unity, TDD per module.
4. **Bug-free**: fix the defects found during exploration, each pinned by a regression test.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope cycle 1 | `common/` only |
| Memory model | **Static / caller-provided storage, never `malloc`** |
| OOP style | **Transparent struct + `self`-methods**, fields private-by-convention |
| Test framework | **Unity (ThrowTheSwitch)**, vendored, host-run |
| `log` lib | **Light touch** — test + bug-fix only, no OOP rewrite (it is an rxi fork) |
| Folder rename | Yes (`dll`, `ringbuf`) + delete stale `Debug/` dirs and tracked ELF binaries |
| `ringbuf` semantics | Replace linked-list-of-strings with a **generic flat-array FIFO**; drop the substring-search feature |
| Fake locking | Remove the cooperative `isBusy` flag; document an honest ISR-guard contract |

## Target layout

```
common/
├── third_party/unity/        # vendored Unity (MIT): unity.c, unity.h, unity_internals.h
├── dll/
│   ├── dll.h  dll.c
│   └── test_dll.c
├── ringbuf/
│   ├── ringbuf.h  ringbuf.c
│   └── test_ringbuf.c
├── str_utils/
│   ├── str_utils.h  str_utils.c
│   └── test_str_utils.c
├── log/
│   ├── src/log.c  src/log.h          # rxi fork, kept (light touch)
│   ├── example/main.c
│   └── test_log.c
├── Makefile                  # `make test` = build + run all host tests; also size/strict-compile checks
├── README.md  LICENSE
```

Removed: `dlinklist.c/`, `ringbuffchar.c/` folder names; all `Debug/` dirs; tracked Linux-ELF build artifacts.

## Module specs

### `dll` — doubly linked list over a static node pool

Caller provides a `dll_node[]` pool; the list maintains a free-list inside it. O(1) push/pop, fixed capacity = pool size, zero heap.

```c
typedef enum { DLL_OK, DLL_FULL, DLL_EMPTY, DLL_NOT_FOUND, DLL_INVALID } dll_err;

typedef struct dll_node { void *value; struct dll_node *prev, *next; } dll_node;
typedef struct { dll_node *pool, *head, *tail, *free; uint16_t count, cap; } dll_list;

void     dll_init(dll_list *self, dll_node *pool, uint16_t cap);
dll_err  dll_push_head(dll_list *self, void *value);
dll_err  dll_push_tail(dll_list *self, void *value);
dll_err  dll_pop_head (dll_list *self, void **out);
dll_err  dll_pop_tail (dll_list *self, void **out);
dll_err  dll_find(const dll_list *self, const void *key,
                  int (*match)(const void *a, const void *b), dll_node **found);
uint16_t dll_count(const dll_list *self);
bool     dll_is_empty(const dll_list *self);
bool     dll_is_full (const dll_list *self);
void     dll_clear(dll_list *self);
```

- Stores opaque `void*` values; never copies/frees the payload (caller owns it).
- `match` returns 0 on equal (strcmp-style); `dll_find` returns `DLL_OK` + `*found` on hit, `DLL_NOT_FOUND` otherwise.
- NULL-arg guard → `DLL_INVALID`.

### `ringbuf` — generic fixed-capacity FIFO over a flat array

```c
typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;

typedef struct {
    uint8_t *buf;       /* caller storage: cap * elem_size bytes */
    uint16_t cap;       /* element capacity */
    uint16_t elem_size; /* bytes per element (1 = classic byte/UART buffer) */
    uint16_t head;      /* index of oldest element */
    uint16_t count;     /* elements currently stored */
    bool     overwrite; /* full behavior: overwrite oldest vs reject */
} ringbuf;

void     ringbuf_init(ringbuf *self, void *storage, uint16_t cap, uint16_t elem_size, bool overwrite);
rb_err   ringbuf_put (ringbuf *self, const void *elem); /* memcpy elem_size bytes in; RB_OVERWRITE if it evicted oldest, RB_FULL if !overwrite */
rb_err   ringbuf_get (ringbuf *self, void *out);        /* memcpy oldest out; RB_EMPTY if none */
rb_err   ringbuf_peek(const ringbuf *self, void *out);  /* like get without removing */
uint16_t ringbuf_count(const ringbuf *self);
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full (const ringbuf *self);
void     ringbuf_clear(ringbuf *self);
```

- **Value semantics**: elements are copied in/out via `memcpy` — no pointer ownership, no `freefunc`, no `malloc`.
- `overwrite=true`: when full, `put` evicts the oldest and returns `RB_OVERWRITE`. `overwrite=false`: `put` returns `RB_FULL` and stores nothing.
- Substring-search feature of the old `ringbuffchar` is intentionally removed; string work belongs in `str_utils`.

### `str_utils` — freestanding string helpers

Keep the freestanding helpers (no libc string dependency, MCU-friendly):

```c
uint16_t str_len(const char *s);
int      str_compare_n(const char *a, const char *b, uint16_t n); /* strcmp-style int, no truncation */
char    *str_cpy_n(char *dst, const char *src, uint16_t n);
void     str_reverse(char *s);                                   /* uses str_len, not libc strlen */
```

Fixes: `str_reverse` no longer calls libc `strlen` without `<string.h>`; `str_compare_n` returns a proper `int` (no `uint8_t` truncation).

### `log` — light touch (rxi fork kept)

- No structural rewrite. Keep `src/log.{c,h}` + `example/main.c`.
- Add Unity tests: level filtering (`log_set_level`), `log_set_quiet`, callback fires (`log_add_callback`), the local per-message `tag` threading, `log_add_fp(FILE*, level)`.
- Fix any genuine defect surfaced by the tests.
- Document MCU usage: register a `log_add_callback` that writes the formatted line to UART instead of relying on the `FILE*`/`stderr` sink; `<time.h>` timestamps may be stubbed on bare metal.

## Conventions

- **No fake thread-safety.** Remove `isBusy`. Header docs each container as "not reentrant; if shared with an ISR, wrap the call in `ATOMIC_BLOCK`/`cli`."
- Per-module, clearly-prefixed error enums (above).
- Reusable libs (`dll`, `ringbuf`, `str_utils`) include only `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, and `<string.h>` (for `memcpy`, provided by avr-libc) — **no host-only headers**, so they cross-compile for AVR unchanged. `log` remains hosted by design.
- Consistent include guards (`CLIBS_DLL_H`, …), doc comments on every public function.

## Bugs to fix (each gets a regression test)

1. `dlinklist` `list_tput`: duplicated busy-check, never sets `isBusy` → broken lock. (Resolved by removing `isBusy`.)
2. `dlinklist` header guard mismatch (`SRC_DLINKLIST_H__` vs `SRC_DLINKLIST_H_H_`) + malformed `#endif`.
3. `dlinklist` test comparator UB: `int(char*,char*)` passed where `int(void*,void*)` expected → runtime **segfault** on modern toolchains. (Resolved by the `match` signature.)
4. `dlinklist` test `printf` format mismatches (`%p` for `uint32_t`, `%s` for `void*`).
5. `str_utils` `str_reverse` calls `strlen` with no `<string.h>` (implicit declaration).
6. `str_utils` `str_compare_n` truncates `int` diff into `uint8_t`.

## Testing & build

- **Unity**, vendored under `third_party/unity/`, host-run. One `test_<module>.c` per library, each its own `main()` running a Unity suite.
- TDD per module: write tests → RED → implement → GREEN.
- `Makefile` targets:
  - `make test` — compile each suite against its lib + Unity, run all, report pass/fail.
  - `make strict` — compile the reusable libs with `-std=c11 -Wall -Wextra -Werror` (and conceptually AVR-clean: no host headers).
- Success = all suites green; reusable libs compile with no warnings under strict flags.

## Out of scope (this cycle)

- `avr-libs` modules (need register mocking / simavr — separate cycle, by module group).
- `esp-idf-template` (hello-world skeleton, nothing to test).
- OOP rewrite of `log`.
- A real ISR-safe lock-free SPSC ring buffer (documented contract only for now).

## Build sequence (high level)

1. Vendor Unity + add `Makefile` skeleton.
2. `str_utils` (smallest, no deps) → test → fix → green.
3. `ringbuf` → test → implement → green.
4. `dll` → test → implement → green.
5. `log` → tests + bug-fix (light touch) → green.
6. Remove stale `Debug/`/ELF, rename folders, update `common/README.md`, strict-compile check.
7. Commit per module; final review.
