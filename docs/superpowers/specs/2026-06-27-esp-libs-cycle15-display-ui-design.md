# esp-libs cycle 15 — Display/UI: ssd1306_gfx + tm1637 + lcd1602 Design

**Status:** approved (brainstorm 2026-06-27)

**Goal:** Add three independent L3 display/UI items to esp-libs — `ssd1306_gfx` (OLED graphics primitives), `tm1637` (4-digit 7-seg), `lcd1602` (16×2 char LCD over PCF8574) — each header-split into a host-Unity-tested pure part + SDK glue (`ssd1306_gfx` is pure-only).

## Background

esp-libs has displays `ssd1306` (1bpp framebuffer + 5×7 text) and `max7219` (7-seg/matrix), but no line/rect graphics, no 4-digit 7-seg, and no character LCD. This cycle rounds out the node UI. `ssd1306_gfx` stacks on the cycle-11 `ssd1306_fb` (pure-on-pure, co-habiting `esp-libs/ssd1306/` exactly like `ssd1306_text` from cycle 12). `lcd1602` uses the existing raw `i2c_bus_write`; `tm1637` is a 2-wire GPIO bit-bang. No bus prerequisite.

**Naming convention (cycle 13+):** pure part = `<lib>.{h,c}` (freestanding, the TU named in `test_<lib>.c`, no SDK header); SDK glue = `<lib>_dev.{h,c}`. (`ssd1306_gfx` is pure-only — no `_dev`.)

**Engine note:** first sprint to run on the patched embedded-workflow engine (Integrator produces a README row per lib + pathspec commits). Verify both during integration/review.

## Architecture

Three independent L3 stories run in parallel under embedded-sprint (`deps: []`), each header-split: pure `<lib>.{h,c}` (host-Unity-tested) + SDK glue `<lib>_dev.{h,c}` (xtensa COMPILE_GATE **deferred**) — except `ssd1306_gfx`, which is pure-only. Integration wires `esp-libs/Makefile` (test 35 → **38**), `component.mk`, `esp-gate/main/main.c`, `README`.

### Conventions (per profile-clibs)

- Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable. `ssd1306_gfx` also includes the pure `ssd1306_fb.h`.
- Glue files include `esp_err.h` + `i2c_bus.h` (lcd1602) or `driver/gpio.h`/`rom/ets_sys.h` (tm1637); not host-runnable.
- `CLIBS_<MODULE>_H` guards; NULL-guard every public pointer param; doc comment per public fn; "Not reentrant".
- **No UB:** shifts on positive uint8/uint16 operands only; no left-shift of a possibly-negative value.

## C15-1 `ssd1306_gfx` — graphics primitives (pure-only)

Lives in `esp-libs/ssd1306/ssd1306_gfx.{h,c}` (co-habits the ssd1306 dir, like `ssd1306_text`). Every write funnels through `ssd1306_fb_set_pixel`, which clips out-of-range coords → unbounded coordinates are safe (no OOB).

```c
/* esp-libs/ssd1306/ssd1306_gfx.h — CLIBS_SSD1306_GFX_H, <stdint.h>, <stdbool.h>, "ssd1306_fb.h" */
void ssd1306_draw_hline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, bool on);   /* x..x+w-1 at y */
void ssd1306_draw_vline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t h, bool on);   /* y..y+h-1 at x */
void ssd1306_draw_line(ssd1306_fb *fb, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on); /* integer Bresenham */
void ssd1306_draw_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);     /* outline */
void ssd1306_fill_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);     /* solid */
```
- `w == 0` / `h == 0` / NULL fb → no-op. `draw_rect` = top+bottom hline + left+right vline. `fill_rect` = h hlines.
- Vectors (128×64 fb, reconstruct via `ssd1306_fb_get_pixel`):
  - `hline(2,3,4,true)` → (2,3)(3,3)(4,3)(5,3) on; (1,3)/(6,3) off.
  - `vline(3,2,4,true)` → (3,2)(3,3)(3,4)(3,5) on.
  - `line(0,0,3,3,true)` → (0,0)(1,1)(2,2)(3,3) on.
  - `rect(0,0,4,3,true)` → border on (top y0 x0..3, bottom y2 x0..3, left/right cols); interior (1,1) off.
  - `fill_rect(0,0,2,2,true)` → (0,0)(1,0)(0,1)(1,1) on.
  - `draw_hline(NULL,…)` → no crash.

No glue (pure-only) — no deferred part for this story.

## C15-2 `tm1637` — 4-digit 7-seg (2-wire)

**Pure** `esp-libs/tm1637/tm1637.{h,c}`:
```c
/* 7-seg byte: bit0=a, bit1=b, ... bit6=g, bit7=dp. 0..9 -> pattern; >9 -> 0x00 (blank). */
uint8_t tm1637_digit_segments(uint8_t digit);

/* Render value % 10000 into 4 segment bytes (out[0]=thousands .. out[3]=ones).
 * leading_zeros=false blanks leading zeros (but the ones digit is always shown).
 * NULL out -> no-op. */
void tm1637_encode_number(uint16_t value, bool leading_zeros, uint8_t out[4]);
```
- Segment table: `0→0x3F, 1→0x06, 2→0x5B, 3→0x4F, 4→0x66, 5→0x6D, 6→0x7D, 7→0x07, 8→0x7F, 9→0x6F`.
- Vectors: `digit(8)→0x7F`, `digit(0)→0x3F`, `digit(10)→0x00`; `encode_number(1234,false)→{0x06,0x5B,0x4F,0x66}`; `encode_number(42,false)→{0x00,0x00,0x66,0x5B}`; `encode_number(42,true)→{0x3F,0x3F,0x66,0x5B}`; `encode_number(0,false)→{0x00,0x00,0x00,0x3F}`; NULL out → no-op.

**Glue** `esp-libs/tm1637/tm1637_dev.{h,c}`:
```c
typedef struct { gpio_num_t clk; gpio_num_t dio; uint8_t brightness; } tm1637;   /* brightness 0..7 */
esp_err_t tm1637_init(tm1637 *self, gpio_num_t clk, gpio_num_t dio, uint8_t brightness);
esp_err_t tm1637_show(tm1637 *self, const uint8_t segments[4]);
/* bit-bang 2-wire: start; cmd 0x40 (auto-addr write); stop; start; addr 0xC0; 4 data
 * bytes; stop; start; 0x88|brightness (display on); stop. ACK after each byte. */
```
hardware_deferred: 2-wire bit-bang timing (CLK/DIO setup/hold, ACK sampling) on a real TM1637; xtensa COMPILE_GATE link.

## C15-3 `lcd1602` — 16×2 char LCD over PCF8574 (I2C, 4-bit)

**Pure** `esp-libs/lcd1602/lcd1602.{h,c}`:
```c
/* Build the 4 PCF8574 byte writes that clock one 8-bit HD44780 value in 4-bit mode.
 * PCF8574 bit map: P0=RS, P1=RW(0), P2=EN, P3=backlight, P4..P7=D4..D7.
 * Emits: hi-nibble|EN, hi-nibble, lo-nibble|EN, lo-nibble (EN high-then-low latches
 * each nibble). NULL out -> no-op. */
void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4]);
```
- Layout: `hi = value & 0xF0`; `lo = (value << 4) & 0xF0`; `base = (rs?0x01:0) | (backlight?0x08:0)`; `EN = 0x04`. `out = { hi|base|EN, hi|base, lo|base|EN, lo|base }`.
- Vectors: `encode_byte(0x41,true,true)→{0x4D,0x49,0x1D,0x19}` ('A', data); `encode_byte(0x01,false,true)→{0x0C,0x08,0x1C,0x18}` (clear-display command); `encode_byte(0x41,false,false)→{0x44,0x40,0x14,0x10}` (no RS, no backlight); NULL out → no-op.

**Glue** `esp-libs/lcd1602/lcd1602_dev.{h,c}`:
```c
typedef struct { uint8_t addr; bool backlight; } lcd1602;   /* addr 0x27 or 0x3F */
esp_err_t lcd1602_init(lcd1602 *self, uint8_t addr);           /* HD44780 4-bit init sequence */
esp_err_t lcd1602_command(lcd1602 *self, uint8_t cmd);         /* rs=0 */
esp_err_t lcd1602_putc(lcd1602 *self, char c);                 /* rs=1 */
esp_err_t lcd1602_puts(lcd1602 *self, const char *s);
esp_err_t lcd1602_set_cursor(lcd1602 *self, uint8_t col, uint8_t row);  /* cmd 0x80 | (row?0x40:0)+col */
```
Each command/char → `lcd1602_encode_byte` → `i2c_bus_write(addr, out, 4)` with the EN-pulse settle delays. hardware_deferred: 4-bit init timing + EN-pulse/clear delays on a real PCF8574+HD44780; xtensa COMPILE_GATE link.

## Testing & gates

- 3 new pure Unity suites (`test_ssd1306_gfx`, `test_tm1637`, `test_lcd1602`) → `make -C esp-libs test` **35 → 38**. (`ssd1306_gfx` host suite links `ssd1306_gfx.c` + `ssd1306_fb.c`.)
- `make strict` (-Werror) on every pure `.c`; `make sanitize` (UBSan+ASan, `ASAN_OPTIONS=detect_leaks=0` on darwin) clean.
- tm1637/lcd1602 glue: COMPILE_GATE **deferred** (xtensa absent), substituted by an adversarial reviewer pass (2-wire protocol / PCF8574 framing / include resolution / NULL-safety).

## Out of scope (deferred)

- ssd1306_gfx circle/triangle; tm1637 colon + scrolling text; lcd1602 custom CGRAM chars + display scroll — "fuller" scope deferred per the lean-core decision.
- On-hardware run / timing validation for all three.
- Other displays (ST7735/ILI9341 SPI TFT, etc.) — future cycles.

## Execution

1. `writing-plans` → implementation plan for the 3 items (no Task-0 prereq).
2. `embedded-sprint` → 3 parallel chains → Integration (test→38) → Sprint Review → Retro → merge (user-gated) → push → memory update. First run on the patched engine — confirm README rows produced + per-lib commit labels correct.
