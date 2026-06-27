# esp-libs cycle 11 — L3 drivers `ssd1306` + `max7219` + `rotary` (design)

Date: 2026-06-27
Location: `clibs/esp-libs/` (the ESP8266 component) + `clibs/esp-gate/` (compile-gate).

## Context

Three more L3 drivers — a node-UI set: an OLED display (on `i2c_bus`), an LED 7-seg/matrix display
(on `spi_bus` — the **first L3 on spi_bus**), and a rotary-encoder input. Each follows the L3
header-split (pure logic file host-Unity-tested + SDK glue compile-gated). esp-libs `make test`
22 → **25 suites**; pure suites run under `make sanitize`.

- **`ssd1306`** — 128×64 SSD1306 OLED. Pure: 1bpp page-addressed framebuffer + pixel ops.
- **`max7219`** — MAX7219 8-digit 7-seg / 8×8 matrix. Pure: command-frame + 7-seg segment encode.
- **`rotary`** — quadrature rotary encoder. Pure: table-driven quarter-step decode + position.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Batch | `ssd1306`, `max7219`, `rotary` (independent; each a pure suite + deferred glue). |
| `ssd1306` scope | framebuffer + set/get/fill/clear pixel only — **no text/font this cycle** (YAGNI). The buffer IS the SSD1306 page layout (byte = column within a page, bit = row within page), so glue streams it directly. |
| `max7219` | pure = `max7219_frame` (16-bit reg+data → 2 bytes MSB-first) + `max7219_digit_segments` (0..9 → no-decode segment byte, bit7=DP/b6=A..b0=G); glue uses `spi_bus`. |
| `rotary` | table-driven **quarter-step** decode (position counts quarter-steps, ~4 per detent); `rotary_dir` NONE/CW/CCW. |
| Glue | `ssd1306` over i2c_bus, `max7219` over spi_bus, `rotary_gpio` 2× GPIO — all **COMPILE_GATE deferred** (xtensa absent). |
| Out of scope | text/font rendering, SSD1306 SPI variant, MAX7219 BCD-decode mode + cascading >1 chip, rotary push-button, on-hardware run. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: +3 host-test targets (test -> 25) + 3 strict lines (sanitize unchanged)
├── component.mk    # CHANGED: +3 dirs (3 lists) + 6 .o
├── ssd1306/   ssd1306_fb.{h,c}      ssd1306.{h,c}        test_ssd1306_fb.c
├── max7219/   max7219_encode.{h,c}  max7219.{h,c}        test_max7219_encode.c
└── rotary/    rotary.{h,c}          rotary_gpio.{h,c}    test_rotary.c
clibs/esp-gate/main/main.c   # CHANGED: include the 3 glue headers + touch one symbol each
```

Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, host-testable. Glue:
`esp_err.h` + `i2c_bus.h`/`spi_bus.h`/`driver/gpio.h`. Conventions = clibs standard (transparent struct +
`self`-methods, `CLIBS_*_H` guards, NULL guards, doc comment per public fn, "Not reentrant", no malloc,
**no UB** — shifts on `uint8_t`/`uint32_t` positive only).

## Module specs

### `ssd1306_fb` (pure) — `CLIBS_SSD1306_FB_H`, `<stdint.h>`,`<stdbool.h>`,`<stddef.h>`
```c
typedef struct { uint8_t *buf; uint8_t width; uint8_t height; } ssd1306_fb;
#define SSD1306_FB_BYTES(w, h) ((size_t)(w) * (h) / 8u)   /* size the caller's buffer */

void ssd1306_fb_init(ssd1306_fb *self, uint8_t *buf, uint8_t width, uint8_t height);  /* clears buf */
void ssd1306_fb_set_pixel(ssd1306_fb *self, uint8_t x, uint8_t y, bool on);           /* OOB -> no-op */
bool ssd1306_fb_get_pixel(const ssd1306_fb *self, uint8_t x, uint8_t y);              /* OOB/NULL -> false */
void ssd1306_fb_fill(ssd1306_fb *self, bool on);
void ssd1306_fb_clear(ssd1306_fb *self);
```
- Page-addressed layout: `idx = (y/8)*width + x`, `bit = y%8` (bit0 = top row of the page). The byte
  buffer is exactly what the SSD1306 GDDRAM expects, so the glue streams it unchanged.
- Acceptance (128×64, `uint8_t buf[SSD1306_FB_BYTES(128,64)]` = 1024 bytes): `set_pixel(0,0,true)` →
  `buf[0]==0x01`; `set_pixel(0,7,true)` → `buf[0]` bit7 set; `set_pixel(0,8,true)` → `buf[128]==0x01`
  (page 1); `set_pixel(127,63,true)` → `buf[1023]==0x80`. `get_pixel` round-trips each. `set_pixel(_,_,false)`
  clears. Out of range `set_pixel(128,0)/(0,64)` no-op + `get_pixel` false. `fill(true)` → every
  `get_pixel` true (buf all 0xFF); `clear` → all false. NULL self: set no-op, get false.

### `ssd1306` (glue, deferred) — `CLIBS_SSD1306_H`, `"esp_err.h"`,`"ssd1306_fb.h"`
```c
#define SSD1306_ADDR 0x3C
typedef struct { uint8_t addr; ssd1306_fb fb; } ssd1306;
/* buf is caller storage >= SSD1306_FB_BYTES(width,height). Runs the SSD1306 init command sequence. */
esp_err_t ssd1306_init(ssd1306 *self, uint8_t addr, uint8_t *buf, uint8_t width, uint8_t height);
esp_err_t ssd1306_show(ssd1306 *self);   /* set addressing + write the framebuffer over i2c_bus */
```
Uses `i2c_bus_write_reg` (control byte 0x00 for commands, 0x40 for data). Not host-runnable.

### `max7219_encode` (pure) — `CLIBS_MAX7219_ENCODE_H`, `<stdint.h>`,`<stdbool.h>`
```c
/* Build a 16-bit MAX7219 command (register addr + data) into 2 bytes, MSB-first (out[0]=reg, out[1]=data). */
void    max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2]);
/* Decimal digit 0..9 -> no-decode segment byte (bit7=DP, b6=A, b5=B, b4=C, b3=D, b2=E, b1=F, b0=G);
 * any other value (e.g. 0x0F) -> blank (0x00, +DP). dp ORs in bit7. */
uint8_t max7219_digit_segments(uint8_t value, bool dp);
```
- Segment table (no-decode): `0=0x7E, 1=0x30, 2=0x6D, 3=0x79, 4=0x33, 5=0x5B, 6=0x5F, 7=0x70, 8=0x7F, 9=0x7B`.
- Acceptance: `max7219_frame(0x01, 0x7E, out)` → `out=={0x01,0x7E}`. `digit_segments(0,false)==0x7E`,
  `(1,false)==0x30`, `(8,false)==0x7F`, `(8,true)==0xFF`, `(9,false)==0x7B`, `(0x0F,false)==0x00`,
  `(0x0F,true)==0x80`.

### `max7219` (glue, deferred) — `CLIBS_MAX7219_H`, `"esp_err.h"`,`"max7219_encode.h"`
```c
typedef struct { uint8_t dummy; } max7219;   /* spi_bus is a singleton; no per-instance pin */
/* shutdown-off, scan-limit 7, intensity, decode-mode off — all via spi_bus 16-bit frames. */
esp_err_t max7219_init(max7219 *self, uint8_t intensity);
esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp);   /* pos 0..7 */
esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits);               /* 8x8 matrix row */
```
Each call builds a frame via `max7219_frame` and sends the 2 bytes with `spi_bus_transfer`. Not host-runnable.

### `rotary` (pure) — `CLIBS_ROTARY_H`, `<stdint.h>`
```c
typedef enum { ROTARY_NONE, ROTARY_CW, ROTARY_CCW } rotary_dir;
typedef struct { uint8_t prev_ab; int32_t position; } rotary;   /* fields private */

void       rotary_init(rotary *self, uint8_t ab);    /* ab = (a<<1)|b, the current A/B level */
/* Feed the new (a<<1)|b. A valid Gray transition counts +1 (CW) / -1 (CCW) into position and returns
 * ROTARY_CW/ROTARY_CCW; same state or a 2-step jump returns ROTARY_NONE (position unchanged).
 * NULL self -> ROTARY_NONE. */
rotary_dir rotary_update(rotary *self, uint8_t ab);
int32_t    rotary_position(const rotary *self);      /* 0 if NULL */
```
- Transition table indexed by `(prev_ab<<2)|ab`:
  `{0,+1,-1,0, -1,0,0,+1, +1,0,0,-1, 0,-1,+1,0}` (quarter-step Gray decode).
- Acceptance: `init(0)`; CW cycle `update(1),update(3),update(2),update(0)` → each `ROTARY_CW`, `position==4`.
  Fresh `init(0)`; CCW `update(2),update(3),update(1),update(0)` → each `ROTARY_CCW`, `position==-4`.
  `init(0)`; `update(3)` (2-step jump 00→11) → `ROTARY_NONE`, `position==0`. `update(0)` (same state) →
  `ROTARY_NONE`. NULL self: `rotary_update(NULL,1)`→`ROTARY_NONE`, `rotary_position(NULL)`→0.

### `rotary_gpio` (glue, deferred) — `CLIBS_ROTARY_GPIO_H`, `"esp_err.h"`,`"rotary.h"`
```c
typedef struct { rotary enc; gpio_num_t pin_a; gpio_num_t pin_b; } rotary_gpio;
esp_err_t  rotary_gpio_init(rotary_gpio *self, gpio_num_t pin_a, gpio_num_t pin_b);
rotary_dir rotary_gpio_poll(rotary_gpio *self);   /* read A/B -> (a<<1)|b -> rotary_update */
```

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `ssd1306_fb max7219_encode rotary` to `.PHONY` + the `test:` aggregate
  (→ **25**), a run target each, and a `-Werror` strict line per pure `.c`. `sanitize` unchanged.
- **`esp-libs/component.mk`:** add `ssd1306 max7219 rotary` to `COMPONENT_ADD_INCLUDEDIRS` +
  `COMPONENT_SRCDIRS`, and `ssd1306/ssd1306_fb.o ssd1306/ssd1306.o max7219/max7219_encode.o
  max7219/max7219.o rotary/rotary.o rotary/rotary_gpio.o` to `COMPONENT_OBJS` (tests excluded).
- **`esp-gate/main/main.c`:** include `ssd1306.h`/`max7219.h`/`rotary_gpio.h` + a touch block (pure
  symbols on zeroed buffers + one glue init each).
- **`esp-libs/README.md`:** add 3 L3 rows.

## Testing
- `cd esp-libs && make test` → **25 suites** (22 + ssd1306_fb/max7219_encode/rotary), all `0 Failures`.
- `cd esp-libs && make strict` → `strict: OK`.
- `cd esp-libs && ASAN_OPTIONS=detect_leaks=0 make sanitize` → all 25 clean under UBSan+ASan.
- Glue `COMPILE_GATE` (esp-gate xtensa link) — **DEFERRED** (toolchain absent); re-run on an SDK host.

## Conventions & scope
- L3 header-split: pure logic host-tested + SDK glue compile-gated. No malloc; transparent struct +
  `self`-methods; `CLIBS_*_H` guards; NULL/edge guards; doc comment per public fn; "Not reentrant"; no UB.
- Out of scope: text/font, SSD1306 SPI, MAX7219 BCD-decode + cascading, rotary push-button, hardware run.

## Build sequence
Each driver independent (pure suite + deferred glue) → built in parallel by `embedded-sprint`
(Tester RED on the pure part → Dev GREEN pure + glue → Reviewer + sanitize), then integration wires
`Makefile`/`component.mk`/`esp-gate`/`README` (→ 25) and runs `test` + `strict` + `sanitize`.
1. `ssd1306` 2. `max7219` 3. `rotary` — each RED→GREEN→commit.
4. Integration; `make test`/`strict`/`sanitize`; esp-gate link deferred.
5. clibs feature branch `feat/esp-libs-cycle11-l3` → merge → push (user's call).
