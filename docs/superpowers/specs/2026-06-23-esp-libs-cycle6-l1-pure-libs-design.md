# esp-libs cycle 6 — Layer-1 pure libs: `hysteresis` + `crc` + `map_range` + `median` + `throttle` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project).

## Context

`clibs` is **libraries only**. After the L2 wrappers and the first L3 drivers, the highest-value
hardware-free work is more **Layer-1 pure logic** — fully host-Unity-testable, no SDK/hardware, the
building blocks the drivers and the (separate) node app will consume. This cycle adds five small,
independent pure libs:

- **`hysteresis`** — Schmitt-trigger / deadband on-off control (e.g. `dht`→`relay` thermostat without relay chatter).
- **`crc`** — `crc8_maxim` (Dallas/Maxim 1-Wire, needed by a future `ds18b20`) + `crc16_modbus`.
- **`map_range`** — integer linear map + clamp (ADC counts → real units).
- **`median`** — `median3` despike (complements the EMA `filter`).
- **`throttle`** — minimum interval between events (telemetry publish rate-limit).

All are freestanding portable C (host-tested today, compile on ESP8266 unchanged). For ship-consistency
they are also added to the `esp_libs` component + esp-gate symbol-touch (trivially cross-compile-clean).

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | the five libs above. ds18b20/other L3, CI/quality work, node_core = later. |
| Test | host-Unity (real RED→GREEN) for all five; + component.mk/esp-gate for ship-consistency. |
| `median` | `median3` only (median-of-3, stateless) — YAGNI, no ring buffer / sort. |
| `hysteresis` | rising-edge Schmitt: output ON at `>= high`, OFF at `<= low`, hold in the deadband. |
| `crc` | `crc8_maxim` (poly 0x8C reflected, init 0) + `crc16_modbus` (poly 0xA001, init 0xFFFF). |
| Out of scope | ds18b20, more L3 drivers, CI/coverage/fuzz, node_core app. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: +5 host-test targets (test -> 14 suites) + strict for the 5
├── component.mk    # CHANGED: +5 dirs in all 3 lists + 5 .o in OBJS
├── <existing dirs> # unchanged
├── hysteresis/  hysteresis.{h,c}  test_hysteresis.c
├── crc/         crc.{h,c}         test_crc.c
├── map_range/   map_range.{h,c}   test_map_range.c
├── median/      median.{h,c}      test_median.c
└── throttle/    throttle.{h,c}    test_throttle.c
clibs/esp-gate/main/main.c   # CHANGED: touch the 5 symbols
```

All five are pure C (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>` only; no SDK headers), self-contained,
no malloc, no cross-dependency. Conventions = clibs standard (`CLIBS_*_H` guards, transparent struct +
`self`-methods where stateful, doc comment per public function, NULL/edge guards, "Not thread-safe").

## Module specs

### `hysteresis` (`CLIBS_HYSTERESIS_H`, `<stdint.h>`,`<stdbool.h>`)
```c
typedef struct { int32_t low; int32_t high; bool on; } hysteresis;
void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial); /* if low>high, swap */
bool hysteresis_update(hysteresis *self, int32_t value);  /* >=high -> on; <=low -> off; in (low,high) -> hold; returns on */
bool hysteresis_state(const hysteresis *self);            /* current output (false if NULL) */
```
- Tests: `init(20,25,false)`; `update(22)`→false (deadband holds initial); `update(26)`→true (≥high);
  `update(24)`→true (deadband holds); `update(20)`→false (≤low); `update(25)`→true (≥high). `init(25,20,…)`
  swaps to low=20,high=25. NULL: `hysteresis_update(NULL,x)`→false, `hysteresis_state(NULL)`→false.

### `crc` (`CLIBS_CRC_H`, `<stdint.h>`,`<stddef.h>`)
```c
uint8_t  crc8_maxim(const uint8_t *data, size_t len);    /* CRC-8/MAXIM (poly 0x8C reflected, init 0x00) */
uint16_t crc16_modbus(const uint8_t *data, size_t len);  /* CRC-16/MODBUS (poly 0xA001, init 0xFFFF) */
```
- Bytewise reflected algorithm (table-free, MCU-light). NULL data → returns the init value (0 / 0xFFFF).
- Tests (catalogue check values for input ASCII `"123456789"`, len 9):
  `crc8_maxim`→`0xA1`; `crc16_modbus`→`0x4B37`. Plus: empty (`len 0`)→ init (`0x00` / `0xFFFF`); NULL→init.

### `map_range` (`CLIBS_MAP_RANGE_H`, `<stdint.h>`)
```c
int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi);
```
- `map_range`: `out_min + (x-in_min)*(out_max-out_min)/(in_max-in_min)` using an `int64_t` intermediate
  (overflow-safe); `in_min==in_max` → `out_min` (no divide-by-zero). No clamping (caller clamps if needed).
- `clamp_i32`: `x<lo→lo`, `x>hi→hi`, else `x` (assumes `lo<=hi`).
- Tests: `map_range(512,0,1023,0,100)`→50; `(0,0,1023,0,100)`→0; `(1023,0,1023,0,100)`→100; inverted out
  `(0,0,10,100,0)`→100; `in_min==in_max`→out_min; `clamp_i32(150,0,100)`→100, `(-5,0,100)`→0, `(50,0,100)`→50.

### `median` (`CLIBS_MEDIAN_H`, `<stdint.h>`)
```c
int32_t median3(int32_t a, int32_t b, int32_t c);   /* the middle of three (despike) */
```
- Stateless; standard 3-compare median. Tests: `(1,2,3)`→2; `(3,1,2)`→2; `(5,5,1)`→5; `(-3,-1,-2)`→-2;
  `(7,7,7)`→7.

### `throttle` (`CLIBS_THROTTLE_H`, `<stdint.h>`,`<stdbool.h>`)
```c
typedef struct { uint32_t interval_ms; uint32_t last_ms; bool primed; } throttle;
void throttle_init(throttle *self, uint32_t interval_ms);
bool throttle_allow(throttle *self, uint32_t now_ms);  /* first call -> true; else (now-last)>=interval -> true (+update); caller supplies now_ms */
```
- First call (`!primed`) → prime + return true. Else `(uint32_t)(now_ms - last_ms) >= interval_ms` →
  update `last_ms` + true, else false. The unsigned subtraction is correct across a `uint32` wrap.
- Tests: `init(1000)`; `allow(0)`→true; `allow(500)`→false; `allow(1000)`→true; `allow(1500)`→false;
  `allow(2000)`→true. Wrap: `init(100)`, prime at `now=0xFFFFFFF0`, then `allow(0x00000005)` (elapsed 21) → true.
  NULL: `throttle_allow(NULL,x)`→false.

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `hysteresis crc map_range median throttle` to `.PHONY` and the `test:`
  aggregate (→ 14 suites), one run target each, and a `strict:` `-Werror` compile of each `<lib>.c`.
- **`esp-libs/component.mk`:** add the 5 dirs to `COMPONENT_ADD_INCLUDEDIRS` + `COMPONENT_SRCDIRS`, and
  `hysteresis/hysteresis.o crc/crc.o map_range/map_range.o median/median.o throttle/throttle.o` to
  `COMPONENT_OBJS`. `test_*.c` excluded; `COMPONENT_REQUIRES` unchanged.
- **`esp-gate/main/main.c`:** include the 5 headers + a touch block calling one symbol of each.

## Testing
- **Host:** `cd esp-libs && make test` → **14 suites** (the 9 existing + the 5 new), all `0 Failures`;
  `make strict` → `strict: OK`.
- **ESP compile-gate:** `cd esp-gate && make` → links `build/esp_libs_gate.elf` (the 5 pure libs are
  freestanding, so this is a formality, but keeps the component consistent).

## Conventions & scope
- Freestanding pure C, no SDK headers, no malloc, transparent structs + `self`-methods, `CLIBS_*_H`
  guards, NULL/edge guards, doc comment per public function, "Not thread-safe".
- Out of scope: ds18b20/other L3 drivers, CI/coverage/fuzz/static-analysis, node_core app.

## Build sequence
Each lib is independent (own dir, own host test). The five can be developed in parallel; a single
integration step then wires all five into `Makefile` + `component.mk` + `esp-gate/main.c` and runs the
full host gate (14 suites) + esp-gate link.
1. `hysteresis` — test → impl → green.
2. `crc` — test → impl → green.
3. `map_range` — test → impl → green.
4. `median` — test → impl → green.
5. `throttle` — test → impl → green.
6. Integration: wire all 5 (Makefile/component.mk/esp-gate); `make test` (14 suites) + `make strict` +
   esp-gate link; README L1 table update.
7. (clibs feature branch → merge → push.)
