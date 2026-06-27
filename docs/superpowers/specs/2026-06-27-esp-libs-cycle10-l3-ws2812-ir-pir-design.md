# esp-libs cycle 10 — L3 drivers `ws2812` + `ir_nec` + `pir` (design)

Date: 2026-06-27
Location: `clibs/esp-libs/` (the ESP8266 component) + `clibs/esp-gate/` (compile-gate).

## Context

Three more L3 drivers, each with a meaty **host-testable pure part** (the real deliverable, since the
SDK glue stays xtensa-deferred) — a smart-home interaction set: RGB light out, IR remote in, motion in.
Follows the established L3 header-split (pure logic file host-Unity-tested + SDK glue compile-gated, like
`ds18b20`/`bme280`). esp-libs `make test` 19 → **22 suites**; all pure suites run under `make sanitize`
(UBSan+ASan, added to `esp-libs/Makefile` in cycle 9).

- **`ws2812`** — WS2812/NeoPixel RGB strip. Pure: render colors → GRB byte buffer with brightness.
- **`ir_nec`** — IR remote receive (NEC). Pure: decode a mark/space duration array → address+command.
- **`pir`** — PIR motion sensor. Pure: retrigger/hold-time FSM with START/END events.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch | `ws2812`, `ir_nec`, `pir` (independent; each a pure suite + deferred glue). |
| `ws2812` order/units | **GRB** byte order (WS2812 wire order); per-channel brightness scale `c*brightness/255`; pure renders into a caller GRB buffer. |
| `ir_nec` validation | validate the **command/~command** complement (always present in NEC); expose `address`=byte0 + `raw` (32-bit) so extended-NEC remotes still decode; do **not** hard-fail on the address complement. |
| `pir` output | event enum `PIR_NONE`/`PIR_MOTION_START`/`PIR_MOTION_END` (like `button`), plus `pir_active`. |
| Glue | `ws2812` 800 kHz bit-bang; `ir_nec_rx` GPIO edge capture; `pir_gpio` GPIO read — all **COMPILE_GATE deferred** (xtensa absent). |
| Out of scope | RGBW/SK6812, RMT/i2s WS2812 backends, NEC repeat-frame handling, RC5/other IR codecs, on-hardware run. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: +3 host-test targets (test -> 22) + 3 strict lines (sanitize unchanged, depends on test)
├── component.mk    # CHANGED: +3 dirs (3 lists) + 6 .o (render/decode/fsm + 3 glue)
├── ws2812/   ws2812_render.{h,c}  ws2812.{h,c}      test_ws2812_render.c
├── ir_nec/   ir_nec.{h,c}         ir_nec_rx.{h,c}   test_ir_nec.c
└── pir/      pir.{h,c}            pir_gpio.{h,c}     test_pir.c
clibs/esp-gate/main/main.c   # CHANGED: include the 3 glue headers + touch one symbol each
```

Pure files: `<stdint.h>`/`<stdbool.h>`/`<stddef.h>` only, no SDK, host-testable. Glue: `esp_err.h` +
`driver/gpio.h` (+ timing). Conventions = clibs standard (transparent struct + `self`-methods,
`CLIBS_*_H` guards, NULL guards, doc comment per public fn, "Not reentrant", no malloc, **no UB** — no
left-shift of a possibly-negative value).

## Module specs

### `ws2812_render` (pure) — `CLIBS_WS2812_RENDER_H`, `<stdint.h>`,`<stddef.h>`
```c
typedef struct { uint8_t r, g, b; } ws2812_rgb;
/* Render n pixels into a GRB byte buffer (3 bytes/pixel: out[3i]=G, [3i+1]=R, [3i+2]=B),
 * each channel scaled by brightness/255. out_size must be >= 3*n. Returns 3*n, or 0 if
 * out too small / NULL / n 0. Pure, host-testable. */
size_t ws2812_render(const ws2812_rgb *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size);
```
- Acceptance: `render({r255,g128,b0}, 1, 255, out, 3)` → `{128,255,0}` returns 3 (GRB). `brightness 128`
  → `{64,128,0}` (128·128/255=64, 255·128/255=128). `brightness 0` → `{0,0,0}`. Two px
  `{(255,0,0),(0,0,255)}, 255` → `{0,255,0, 0,0,255}` returns 6. `out_size 2` (for n 1) → 0. `n 0` → 0.
  NULL px/out → 0.

### `ws2812` (glue, deferred) — `CLIBS_WS2812_H`, `"esp_err.h"`,`"ws2812_render.h"`
```c
typedef struct { gpio_num_t pin; uint16_t count; uint8_t *grb; size_t grb_size; } ws2812;
/* grb_buf is caller storage >= 3*count bytes (no malloc). */
esp_err_t ws2812_init(ws2812 *self, gpio_num_t pin, uint16_t count, uint8_t *grb_buf, size_t grb_size);
/* render px (count pixels) at brightness into grb, then 800kHz bit-bang out the pin. */
esp_err_t ws2812_show(ws2812 *self, const ws2812_rgb *px, uint8_t brightness);
```
- Timing-critical 800 kHz bit-bang (T0H≈350 ns / T1H≈700 ns, reset ≥50 µs) — a first cut, tune on
  hardware. Not host-runnable.

### `ir_nec` (pure decode) — `CLIBS_IR_NEC_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`
```c
typedef struct { uint8_t address; uint8_t command; uint32_t raw; } ir_nec_result;
/* Decode a NEC frame from mark/space durations in microseconds:
 *   us[0]=leading mark (~9000), us[1]=leading space (~4500), then 32 (mark,space) pairs;
 *   data space classified 0 (~562) vs 1 (~1687) by the 1124us midpoint; marks ignored.
 * Requires count >= 66. Header tolerance +/-25% (mark 6750..11250, space 3375..5625).
 * Bits assembled LSB-first into `raw`; address=raw&0xFF, command=(raw>>16)&0xFF.
 * Validates the command complement ((raw>>24)&0xFF == (~command)&0xFF). Returns true + fills out on a
 * valid frame; false on bad header / count<66 / failed complement / NULL. Pure, host-testable. */
bool ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result *out);
```
- Acceptance: a frame for **address 0x00, command 0x16** (bytes addr=0x00, ~addr=0xFF, cmd=0x16,
  ~cmd=0xE9; `raw==0xE916FF00`) → `decode` true, `address==0x00`, `command==0x16`, `raw==0xE916FF00`.
  (The test builds the duration array from the 4 bytes with a helper: `us[0]=9000`, `us[1]=4500`,
  per bit `mark=562`, `space=562` for 0 / `1687` for 1, LSB-first, 66 entries.) A frame with a flipped
  command bit (so `cmd != ~cmd_inv`) → false. Bad header (`us[0]=3000`) → false. `count 10` → false.
  NULL us/out → false.

### `ir_nec_rx` (glue, deferred) — `CLIBS_IR_NEC_RX_H`, `"esp_err.h"`,`"ir_nec.h"`
```c
typedef struct { gpio_num_t pin; uint16_t buf[70]; } ir_nec_rx;
esp_err_t ir_nec_rx_init(ir_nec_rx *self, gpio_num_t pin);
/* Capture one frame's edge timings (bounded by timeout), then ir_nec_decode.
 * ESP_OK / ESP_ERR_TIMEOUT (no frame) / ESP_ERR_INVALID_CRC (decode failed). */
esp_err_t ir_nec_rx_read(ir_nec_rx *self, ir_nec_result *out, uint32_t timeout_ms);
```
- GPIO edge capture (ISR + esp_timer/cycle counts) into `buf`, then decode. Not host-runnable.

### `pir` (pure FSM) — `CLIBS_PIR_H`, `<stdint.h>`,`<stdbool.h>`
```c
typedef enum { PIR_NONE, PIR_MOTION_START, PIR_MOTION_END } pir_event;
typedef struct { uint32_t hold_ms; uint32_t last_ms; bool active; } pir;   /* fields private */

void      pir_init(pir *self, uint32_t hold_ms);
/* Feed the raw sensor level + a monotonic now_ms. motion_raw true -> refresh last_ms; if it was idle,
 * go active and return PIR_MOTION_START. motion_raw false while active and (now-last) >= hold_ms ->
 * go idle and return PIR_MOTION_END. Otherwise PIR_NONE. NULL self -> PIR_NONE. */
pir_event pir_update(pir *self, bool motion_raw, uint32_t now_ms);
bool      pir_active(const pir *self);   /* false if NULL */
```
- Acceptance: `init(1000)`; `update(true,0)`→`PIR_MOTION_START` (active); `update(false,500)`→`PIR_NONE`
  (within hold); `update(true,800)`→`PIR_NONE` (re-trigger, last=800); `update(false,1700)`→`PIR_NONE`
  (1700-800=900<1000); `update(false,1900)`→`PIR_MOTION_END` (1100>=1000, idle); `update(false,2000)`→
  `PIR_NONE`; `update(true,2100)`→`PIR_MOTION_START` again. `pir_active` tracks the state. NULL self:
  `pir_update(NULL,…)`→`PIR_NONE`, `pir_active(NULL)`→false. `now_ms` compared via wrap-safe unsigned
  subtraction `(now - last) >= hold` (uint32).

### `pir_gpio` (glue, deferred) — `CLIBS_PIR_GPIO_H`, `"esp_err.h"`,`"pir.h"`
```c
typedef struct { pir fsm; gpio_num_t pin; bool active_high; } pir_gpio;
esp_err_t pir_gpio_init(pir_gpio *self, gpio_num_t pin, uint32_t hold_ms, bool active_high);
pir_event pir_gpio_poll(pir_gpio *self, uint32_t now_ms);   /* gpio read -> pir_update */
```

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `ws2812_render ir_nec pir` to `.PHONY` + the `test:` aggregate (→ **22**),
  a run target each (e.g. `-Iws2812 … ws2812/test_ws2812_render.c ws2812/ws2812_render.c $(UNITY)/unity.c`),
  and a `-Werror` strict line per pure `.c`. `sanitize` needs no change (depends on `test`).
- **`esp-libs/component.mk`:** add `ws2812 ir_nec pir` to `COMPONENT_ADD_INCLUDEDIRS` + `COMPONENT_SRCDIRS`,
  and `ws2812/ws2812_render.o ws2812/ws2812.o ir_nec/ir_nec.o ir_nec/ir_nec_rx.o pir/pir.o pir/pir_gpio.o`
  to `COMPONENT_OBJS` (test files excluded).
- **`esp-gate/main/main.c`:** include `ws2812.h`/`ir_nec_rx.h`/`pir_gpio.h` + a touch block (pure symbols
  on zeroed buffers + one glue init each) so it links.
- **`esp-libs/README.md`:** add 3 L3 rows.

## Testing
- `cd esp-libs && make test` → **22 suites** (19 + ws2812_render/ir_nec/pir), all `0 Failures`.
- `cd esp-libs && make strict` → `strict: OK`.
- `cd esp-libs && ASAN_OPTIONS=detect_leaks=0 make sanitize` → all 22 clean under UBSan+ASan (darwin needs
  `detect_leaks=0`; cycle-9 note).
- Glue `COMPILE_GATE` (esp-gate xtensa link) — **DEFERRED** (toolchain absent); re-run on an SDK host.

## Conventions & scope
- L3 header-split: pure logic host-tested + SDK glue compile-gated. No malloc; transparent struct +
  `self`-methods; `CLIBS_*_H` guards; NULL/edge guards; doc comment per public fn; "Not reentrant"; no UB.
- Out of scope: RGBW/SK6812, RMT/i2s WS2812, NEC repeat frames, other IR codecs, on-hardware run.

## Build sequence
Each driver independent (pure suite + deferred glue) → built in parallel by `embedded-sprint`
(Tester RED on the pure part → Dev GREEN pure + glue → Reviewer + sanitize), then integration wires
`Makefile`/`component.mk`/`esp-gate`/`README` (→ 22) and runs `test` + `strict` + `sanitize`.
1. `ws2812` 2. `ir_nec` 3. `pir` — each RED→GREEN→commit.
4. Integration; `make test`/`strict`/`sanitize`; esp-gate link deferred.
5. clibs feature branch `feat/esp-libs-cycle10-l3` → merge → push (user's call).
