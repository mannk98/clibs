# esp-libs cycle 1 — Layer-1 pure building blocks (design)

Date: 2026-06-22
Location: **`clibs/esp-libs/`** — committed directly in the `clibs` repo (like `common/`), NOT a submodule.

## Context

User is building a shared ESP8266 library set (`esp-libs`, the analog of `avr-libs`) BEFORE the
smart-home node app. esp-libs is layered: **Layer 1 = pure logic** (host-testable now, no SDK),
Layer 2 = thin ESP8266-RTOS-SDK wrappers (needs SDK), Layer 3 = device drivers (needs hardware).
This cycle delivers Layer 1 only — plain portable C, host-Unity-tested, no toolchain/hardware.
These compile on the host today and on ESP8266 (newlib) unchanged later.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| First layer | Layer 1 — pure logic, host-tested now (no SDK/hardware) |
| Location | `clibs/esp-libs/` directory, committed directly (not a submodule); can be promoted to a submodule later |
| Cycle-1 set | `backoff`, `debounce`, `filter` (EMA), `mqtt_topic` |
| Unity | Reuse `common/third_party/unity` (no separate copy) |
| Conventions | Same as clibs: freestanding, static/no-malloc, transparent struct + `self`-methods, `CLIBS_*_H` guards, Unity TDD, `-Werror` strict gate |
| Out of scope | SDK wrappers (Layer 2), device drivers (Layer 3), the node's concrete MQTT topic *schema* + JSON payloads, any ESP cross-compile gate (added in Layer 2 when the SDK is installed) |

## Layout

```
esp-libs/
├── Makefile           # host tests + strict gate; reuses ../common/third_party/unity
├── backoff/    backoff.{h,c}    test_backoff.c
├── debounce/   debounce.{h,c}   test_debounce.c
├── filter/     filter.{h,c}     test_filter.c
├── mqtt_topic/ mqtt_topic.{h,c} test_mqtt_topic.c
└── README.md
```

## Module specs

### `backoff` — exponential reconnect backoff (`CLIBS_BACKOFF_H`, `<stdint.h>`)
```c
typedef struct { uint32_t base_ms; uint32_t max_ms; uint32_t cur_ms; } backoff;
void     backoff_init(backoff *self, uint32_t base_ms, uint32_t max_ms); /* max<base → max=base; cur=base */
uint32_t backoff_next(backoff *self);   /* returns current delay, then doubles toward max_ms (overflow-safe cap) */
void     backoff_reset(backoff *self);  /* cur=base (call on a successful connect) */
```
- Double step is overflow-safe: `cur = (cur <= max/2) ? cur*2 : max`.
- Tests: base=100,max=1000 → next sequence 100,200,400,800,1000,1000; reset → 100; max<base clamps; NULL guards (next(NULL)→0).

### `debounce` — input debounce (`CLIBS_DEBOUNCE_H`, `<stdint.h>`, `<stdbool.h>`)
```c
typedef enum { DEBOUNCE_NONE, DEBOUNCE_RISING, DEBOUNCE_FALLING } debounce_edge;
typedef struct { uint8_t stable; uint8_t candidate; uint8_t count; uint8_t threshold; } debounce;
void          debounce_init(debounce *self, uint8_t threshold, uint8_t initial); /* threshold 0→1; stable=initial?1:0 */
debounce_edge debounce_update(debounce *self, uint8_t raw);  /* caller samples at a fixed interval */
uint8_t       debounce_level(const debounce *self);          /* current debounced level 0/1 */
```
- `raw` normalized to 0/1. If `raw == stable` → reset count, NONE. Else count consecutive samples of the differing level; when `count >= threshold` → commit (`stable=raw`, count=0) and return RISING (→1) / FALLING (→0). A transient (fewer than threshold) is ignored.
- Tests: threshold=3 from initial 0 → feed 1,1,1 → NONE,NONE,RISING; then 0,0,0 → NONE,NONE,FALLING; bounce 1,0,1 stays NONE; `debounce_level` reflects committed state.

### `filter` — integer EMA (`CLIBS_FILTER_H`, `<stdint.h>`)
```c
typedef struct { int32_t acc; uint8_t shift; } ema;   /* acc holds value << shift (fixed-point) */
void    ema_init(ema *self, uint8_t shift, int32_t initial); /* acc = initial << shift; alpha = 1/2^shift */
int32_t ema_update(ema *self, int32_t sample);               /* acc += sample - (acc>>shift); returns acc>>shift */
```
- Standard fixed-point EMA — converges to a constant input, no float. Document range: keep `|value| << shift` within int32 (fine for ADC 0–1023, shift ≤ 8).
- Tests: shift=2 from 0: update(100)→25, update(100)→43… ; feeding 100 repeatedly converges to 100 and is stable once reached; init at the steady value stays put; NULL guard.

### `mqtt_topic` — schema-agnostic topic helpers (`CLIBS_MQTT_TOPIC_H`, `<stddef.h>`,`<stdbool.h>`,`<string.h>`)
```c
/* Join count parts with '/' into buf[n] (NUL-terminated). Returns length (excl NUL), or -1 on truncation / NULL arg. */
int  mqtt_topic_join(char *buf, size_t n, const char *const *parts, size_t count);
/* MQTT topic-filter match: '+' matches one level, '#' matches the remainder (must be the last level). */
bool mqtt_topic_match(const char *filter, const char *topic);
```
- `join`: writes `parts[0]/parts[1]/…`; truncation (would exceed n-1) → returns -1, `buf` left NUL-terminated/empty; count 0 → empty string, return 0.
- `match`: level-by-level; `+` = exactly one level; `#` = zero-or-more remaining levels (only valid as the final filter level). Exact strings match exactly.
- Tests: join `{"home","kitchen","temp"}`→`"home/kitchen/temp"` (len 17); truncation into a small buffer → -1; match: `home/+/temp` matches `home/kitchen/temp`, not `home/a/b/temp`; `home/#` matches `home/a/b`; `home/#` matches `home`; `+` non-match across levels; exact match/non-match.

## Build & test

`esp-libs/Makefile` (host only; reuses common's Unity):
```makefile
CC     ?= cc
CFLAGS := -std=c11 -Wall -Wextra -g
UNITY  := ../common/third_party/unity
BUILD  := build
.PHONY: test backoff debounce filter mqtt_topic strict clean
test: backoff debounce filter mqtt_topic
# one host-run target per lib (compile test_<m>.c + <m>.c + unity.c, run)
# strict: compile each <m>.c with -std=c11 -Wall -Wextra -Werror -> /dev/null; echo "strict: OK"
```
No ESP cross-compile gate this cycle (no toolchain; the libs are plain portable C). Layer 2 adds the `xtensa-lx106` gate when the SDK is installed.

## Conventions
- Freestanding: `<stdint.h>`/`<stdbool.h>`/`<stddef.h>` only (+`<string.h>` in `mqtt_topic` for `strlen`/`memcpy`/`strncmp`).
- Static / no malloc; transparent struct + `self`-methods; `CLIBS_*_H` guards; doc comment per public function; NULL-arg guards.
- Each lib is self-contained for cycle 1 (no cross-dependency on `common/` yet — that comes when a lib genuinely needs `ringbuf`/`str_utils`).

## Build sequence
1. `esp-libs/` scaffold: `Makefile` (4 targets + strict, reuse common Unity), `.gitignore` (`build/`), `README.md` stub.
2. `backoff` — test → impl → green.
3. `debounce` — test → impl → green.
4. `filter` — test → impl → green.
5. `mqtt_topic` — test → impl → green.
6. Final gates (`make test` 4 suites, `make strict`), flesh out `README.md`.
7. (clibs feature branch → merge to clibs main → push.)

(Scaffold + first lib can be one task; the four libs are small and independent.)
