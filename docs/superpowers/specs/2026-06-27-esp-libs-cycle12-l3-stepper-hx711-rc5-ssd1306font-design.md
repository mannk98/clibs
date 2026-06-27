# esp-libs cycle 12 — L3 `stepper` + `hx711` + `rc5` + `ssd1306` font/text (design)

Date: 2026-06-27
Location: `clibs/esp-libs/` (the ESP8266 component) + `clibs/esp-gate/` (compile-gate).

## Context

Four more L3 additions: a stepper motor, a load-cell ADC, an RC5 IR codec, and a text/font layer on the
cycle-11 OLED. Each follows the L3 header-split (pure logic host-Unity-tested + SDK glue compile-gated) —
**except `ssd1306` font/text, which is pure-only** (it draws into the existing `ssd1306_fb`, so it has no
SDK glue and no deferred part). esp-libs `make test` 25 → **29 suites**; pure suites run under
`make sanitize`.

(Separately decomposed, NOT in this cycle: the `spi_bus` multi-CS refactor — a later direct change.)

- **`stepper`** — 4-wire unipolar (28BYJ-48). Pure: 8-phase half-step sequence + position.
- **`hx711`** — 24-bit load-cell ADC. Pure: 24-bit two's-complement → sign-extended int32.
- **`rc5`** — IR RC5 (Manchester). Pure: decode 28 half-bit samples → 14-bit frame → fields.
- **`ssd1306` font/text** — pure 5×7 ASCII font + `draw_char`/`draw_string` into `ssd1306_fb`.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch | the 4 items above (independent; `ssd1306_text` reuses the existing `ssd1306_fb`). |
| `stepper` mode | **half-step** 8-phase (standard for 28BYJ-48); coil pattern IN1..IN4 = bits 0..3. |
| `hx711` pure scope | just `hx711_parse` (24-bit two's-complement sign-extend); gain selection lives in the glue. |
| `rc5` input model | **28 Manchester half-bit samples** (0/1), logical 1 = `(0,1)` / 0 = `(1,0)`; validate the leading start bit. |
| `ssd1306` font | 5×7 ASCII (0x20–0x7E), column-major (each glyph = 5 column bytes, bit0 = top row); `draw_string` advances 6 px/char. **pure-only, no glue.** |
| Out of scope | stepper full-step/microstep, hx711 gain in pure, RC5x extended decode + repeat, proportional fonts / scaling, the spi_bus refactor, on-hardware run. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: +4 host-test targets (test -> 29) + 4 strict lines (sanitize unchanged)
├── component.mk    # CHANGED: + stepper/hx711/rc5 dirs (3 lists) + their .o + ssd1306/ssd1306_text.o
├── stepper/   stepper.{h,c}      stepper_gpio.{h,c}   test_stepper.c
├── hx711/     hx711.{h,c}        hx711_gpio.{h,c}     test_hx711.c
├── rc5/       rc5.{h,c}          rc5_rx.{h,c}         test_rc5.c
└── ssd1306/   (existing) + ssd1306_text.{h,c}         test_ssd1306_text.c   # pure-only, no new glue
clibs/esp-gate/main/main.c   # CHANGED: include glue headers + touch one symbol each (incl. ssd1306_text pure)
```

Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK; `ssd1306_text` also includes
`ssd1306_fb.h` (pure). Glue: `esp_err.h` + `driver/gpio.h`. Conventions = clibs standard (`CLIBS_*_H`
guards, NULL guards, doc comment per public fn, "Not reentrant", no malloc, **no UB** — shifts on
`uint8`/`uint16`/`uint32` positive operands only).

## Module specs

### `stepper` (pure) — `CLIBS_STEPPER_H`, `<stdint.h>`,`<stdbool.h>`
```c
typedef struct { uint8_t phase; int32_t position; } stepper;   /* fields private */
void    stepper_init(stepper *self);                /* phase 0, position 0 */
uint8_t stepper_step(stepper *self, bool forward);  /* advance one half-step; position += dir; returns the new coil pattern (IN1..IN4 = bits 0..3) */
uint8_t stepper_coils(const stepper *self);         /* current coil pattern without advancing; 0 if NULL */
int32_t stepper_position(const stepper *self);      /* 0 if NULL */
```
- Half-step table (8): `{0x01,0x03,0x02,0x06,0x04,0x0C,0x08,0x09}`. `step(forward)`:
  `phase = forward ? (phase+1)&7 : (phase+7)&7`; `position += forward?1:-1`; return `HALF[phase]`.
- Acceptance: `init` → `coils()==0x01`, `position()==0`. `step(true)`→`0x03`, pos 1; continuing forward
  yields `0x02,0x06,0x04,0x0C,0x08,0x09`, then `0x01` again (8th step), `position()==8`. Fresh `init`;
  `step(false)`→`0x09` (phase wraps to 7), `position()==-1`. NULL self: `step` returns 0, `coils`/`position`
  return 0, `init(NULL)` no-op.

### `stepper_gpio` (glue, deferred) — `CLIBS_STEPPER_GPIO_H`, `"esp_err.h"`,`"stepper.h"`
```c
typedef struct { stepper drv; gpio_num_t in1, in2, in3, in4; } stepper_gpio;
esp_err_t stepper_gpio_init(stepper_gpio *self, gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4);
esp_err_t stepper_gpio_step(stepper_gpio *self, bool forward);   /* step + write the 4 coil GPIOs */
```

### `hx711` (pure) — `CLIBS_HX711_H`, `<stdint.h>`
```c
/* 24-bit big-endian two's-complement (raw[0]=MSB) -> sign-extended int32. NULL -> 0. */
int32_t hx711_parse(const uint8_t raw[3]);
```
- `v = (raw[0]<<16)|(raw[1]<<8)|raw[2]` (in `uint32_t`); if `v & 0x800000` then `v |= 0xFF000000`; return
  `(int32_t)v`.
- Acceptance: `{0x00,0x00,0x00}`→0; `{0x00,0x00,0x01}`→1; `{0x7F,0xFF,0xFF}`→8388607; `{0xFF,0xFF,0xFF}`→
  −1; `{0x80,0x00,0x00}`→−8388608. NULL → 0.

### `hx711_gpio` (glue, deferred) — `CLIBS_HX711_GPIO_H`, `"esp_err.h"`,`"driver/gpio.h"`,`"hx711.h"`
```c
typedef enum { HX711_GAIN_A128 = 1, HX711_GAIN_B32 = 2, HX711_GAIN_A64 = 3 } hx711_gain;  /* extra pulses */
typedef struct { gpio_num_t sck, dout; hx711_gain gain; } hx711_dev;
esp_err_t hx711_dev_init(hx711_dev *self, gpio_num_t sck, gpio_num_t dout, hx711_gain gain);
esp_err_t hx711_dev_read(hx711_dev *self, int32_t *out, uint32_t timeout_ms);  /* bit-bang 24 bits + gain pulses, hx711_parse */
```

### `rc5` (pure) — `CLIBS_RC5_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`
```c
typedef struct { uint8_t toggle; uint8_t address; uint8_t command; uint16_t raw; } rc5_result;
/* Decode RC5 from 28 Manchester half-bit samples (0/1): bit i = (halfbits[2i],halfbits[2i+1]);
 * (0,1)=logical 1, (1,0)=logical 0, anything else -> false. Bits are MSB-first into a 14-bit `raw`
 * (S1 S2 T A4..A0 C5..C0). Validates the leading start bit S1 = 1. address=(raw>>6)&0x1F,
 * command=raw&0x3F, toggle=(raw>>11)&1. Requires count==28. NULL/bad -> false. Pure, host-testable. */
bool rc5_decode(const uint8_t *halfbits, size_t count, rc5_result *out);
```
- Acceptance: a frame for **address 0x14, command 0x35, toggle 0** (`raw==0x3535`) → `decode` true,
  `address==0x14`, `command==0x35`, `toggle==0`, `raw==0x3535`. (The test builds the 28 half-bits from
  `raw` with a helper: MSB-first, each bit → `(0,1)` for 1 / `(1,0)` for 0.) A malformed half-bit pair
  (e.g. `(1,1)`) → false. `count != 28` → false. Leading start bit 0 → false. NULL → false.

### `rc5_rx` (glue, deferred) — `CLIBS_RC5_RX_H`, `"esp_err.h"`,`"driver/gpio.h"`,`"rc5.h"`
```c
typedef struct { gpio_num_t pin; uint8_t halfbits[28]; } rc5_rx;
esp_err_t rc5_rx_init(rc5_rx *self, gpio_num_t pin);
esp_err_t rc5_rx_read(rc5_rx *self, rc5_result *out, uint32_t timeout_ms);  /* sample line every ~889us -> rc5_decode */
```

### `ssd1306_text` (pure, no glue) — `CLIBS_SSD1306_TEXT_H`, `<stdint.h>`,`"ssd1306_fb.h"`
```c
/* 5x7 ASCII font (0x20..0x7E), column-major: each glyph = 5 column bytes, bit0 = top row.
 * Draws into an ssd1306_fb via ssd1306_fb_set_pixel. Unknown/out-of-range char -> blank (space). */
void    ssd1306_draw_char(ssd1306_fb *fb, uint8_t x, uint8_t y, char c);
uint8_t ssd1306_draw_string(ssd1306_fb *fb, uint8_t x, uint8_t y, const char *s);  /* 6px/char advance; returns end x */
```
- Acceptance (128×64 fb): `draw_char(fb,0,0,' ')` sets **no** pixels in the 5×7 cell. `draw_char(fb,0,0,'A')`:
  reconstructing each column `c` as `byte = OR(get_pixel(c,row)<<row for row 0..6)` gives the font's 'A'
  bytes `{0x7C,0x12,0x11,0x12,0x7C}`. `draw_string(fb,0,0,"A")` returns 6 and renders 'A' at x0;
  `draw_string(fb,0,0,"AA")` returns 12 and renders 'A' at both x0 and x6. NULL fb: `draw_char` no-op,
  `draw_string` returns x unchanged.
- The full 5×7 font table (95 glyphs × 5 bytes, with `'A'=={0x7C,0x12,0x11,0x12,0x7C}`) is bundled in the
  implementation (`ssd1306_text.c`).

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `stepper hx711 rc5 ssd1306_text` to `.PHONY` + the `test:` aggregate
  (→ **29**), a run target each (`ssd1306_text` links `ssd1306/ssd1306_text.c` + `ssd1306/ssd1306_fb.c`),
  and a `-Werror` strict line per pure `.c`. `sanitize` unchanged.
- **`esp-libs/component.mk`:** add `stepper hx711 rc5` to `COMPONENT_ADD_INCLUDEDIRS` + `COMPONENT_SRCDIRS`
  and `stepper/stepper.o stepper/stepper_gpio.o hx711/hx711.o hx711/hx711_gpio.o rc5/rc5.o rc5/rc5_rx.o
  ssd1306/ssd1306_text.o` to `COMPONENT_OBJS` (ssd1306 dir already listed; just add the new .o).
- **`esp-gate/main/main.c`:** include `stepper_gpio.h`/`hx711_gpio.h`/`rc5_rx.h` + touch one symbol each,
  plus `ssd1306_draw_string` (pure) on a stack fb.
- **`esp-libs/README.md`:** add rows (3 L3 drivers + the ssd1306 text note).

## Testing
- `cd esp-libs && make test` → **29 suites** (25 + stepper/hx711/rc5/ssd1306_text), all `0 Failures`.
- `cd esp-libs && make strict` → `strict: OK`.
- `cd esp-libs && ASAN_OPTIONS=detect_leaks=0 make sanitize` → all 29 clean under UBSan+ASan.
- Glue `COMPILE_GATE` (esp-gate xtensa link) — **DEFERRED** (toolchain absent). `ssd1306_text` has no glue
  → fully host-verified (no deferred part).

## Conventions & scope
- L3 header-split (pure host-tested + SDK glue compile-gated); `ssd1306_text` pure-only. No malloc;
  transparent struct + `self`-methods; `CLIBS_*_H` guards; NULL/edge guards; doc per public fn; no UB.
- Out of scope: full-step/microstep, hx711 gain in pure, RC5x/repeat, proportional/scaled fonts, the
  spi_bus refactor, on-hardware run.

## Build sequence
Each item independent → built in parallel by `embedded-sprint` (Tester RED → Dev GREEN pure + glue →
Reviewer + sanitize), then integration wires `Makefile`/`component.mk`/`esp-gate`/`README` (→ 29) and runs
`test` + `strict` + `sanitize`.
1. `stepper` 2. `hx711` 3. `rc5` 4. `ssd1306_text` — each RED→GREEN→commit.
5. Integration; `make test`/`strict`/`sanitize`; esp-gate link deferred.
6. clibs feature branch `feat/esp-libs-cycle12-l3` → merge → push (user's call).
