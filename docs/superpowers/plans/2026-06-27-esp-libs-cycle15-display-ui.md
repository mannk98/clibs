# esp-libs cycle 15 — Display/UI (ssd1306_gfx, tm1637, lcd1602) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three independent L3 display/UI items to esp-libs — `ssd1306_gfx` (OLED graphics primitives), `tm1637` (4-digit 7-seg), `lcd1602` (16×2 char LCD over PCF8574) — each host-tested pure part + SDK glue (`ssd1306_gfx` is pure-only).

**Architecture:** Header-split L3: pure `<lib>.{h,c}` (host-Unity-tested) + SDK glue `<lib>_dev.{h,c}` (xtensa COMPILE_GATE deferred). `ssd1306_gfx` is pure-only and co-habits `esp-libs/ssd1306/` (like `ssd1306_text`), drawing through `ssd1306_fb_set_pixel`. `lcd1602` uses the existing raw `i2c_bus_write`; `tm1637` is a 2-wire GPIO bit-bang. No bus prerequisite. Task 4 wires the build.

**Tech Stack:** C11, host `cc` + Unity (`../common/third_party/unity`), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle15-display-ui-design.md`

## Global Constraints

- **Branch `feat/esp-libs-cycle15-display`** (`git checkout -b feat/esp-libs-cycle15-display` before Task 1).
- **Commands run from `clibs/esp-libs/`.** Unity at `../common/third_party/unity`.
- **Naming:** pure part = `<lib>.{h,c}` (freestanding, named in `test_<lib>.c`); glue = `<lib>_dev.{h,c}` (SDK). `ssd1306_gfx` is pure-only (no `_dev`).
- **Pure files** freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`); `ssd1306_gfx` also includes `ssd1306_fb.h`. No SDK, no malloc, host-testable.
- **Glue files** SDK (`esp_err.h`, `i2c_bus.h`, `driver/gpio.h`, `rom/ets_sys.h`) → **not host-runnable; COMPILE_GATE deferred** (xtensa absent).
- **No UB:** shifts on positive uint8/uint16 operands only; no left-shift of a possibly-negative value.
- **Guards** `CLIBS_<MODULE>_H`; doc comment per public fn; NULL-guard every pointer param; "Not reentrant".
- **Flags:** host `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; sanitize `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (`ASAN_OPTIONS=detect_leaks=0` on darwin).
- **Engine note:** first sprint on the patched embedded-workflow engine — the Integrator produces a README row per lib and commits with pathspec; verify both at integration/review.

---

### Task 1: `ssd1306_gfx` — graphics primitives (pure-only, co-habits ssd1306/)

**Files:** Create `esp-libs/ssd1306/ssd1306_gfx.h`, `esp-libs/ssd1306/ssd1306_gfx.c`; Test `esp-libs/ssd1306/test_ssd1306_gfx.c`

**Interfaces:**
- Consumes: `ssd1306_fb`, `ssd1306_fb_init`, `ssd1306_fb_set_pixel`, `ssd1306_fb_get_pixel`, `SSD1306_FB_BYTES` (cycle 11).
- Produces: `ssd1306_draw_hline/_vline/_line/_rect/_fill_rect`.

- [ ] **Step 1: Write the failing test** — `esp-libs/ssd1306/test_ssd1306_gfx.c`

```c
#include "unity.h"
#include "ssd1306_gfx.h"
#include "ssd1306_fb.h"

static uint8_t BUF[SSD1306_FB_BYTES(128, 64)];

void setUp(void) {}
void tearDown(void) {}

static void test_hline(void) {
    ssd1306_fb fb; ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_hline(&fb, 2, 3, 4, true);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 2, 3));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 5, 3));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 1, 3));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 6, 3));
}

static void test_vline(void) {
    ssd1306_fb fb; ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_vline(&fb, 3, 2, 4, true);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 2));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 5));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 3, 6));
}

static void test_line_diagonal(void) {
    ssd1306_fb fb; ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_line(&fb, 0, 0, 3, 3, true);
    for (uint8_t i = 0; i < 4; i++) TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, i, i));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 1));
}

static void test_rect_outline(void) {
    ssd1306_fb fb; ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_rect(&fb, 0, 0, 4, 3, true);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 2));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 3, 2));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 1, 1));   /* interior empty */
}

static void test_fill_rect(void) {
    ssd1306_fb fb; ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_fill_rect(&fb, 0, 0, 2, 2, true);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 1, 0));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 1));
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 1, 1));
}

static void test_null(void) {
    ssd1306_draw_hline(NULL, 0, 0, 4, true);
    ssd1306_draw_line(NULL, 0, 0, 3, 3, true);   /* no crash */
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hline);
    RUN_TEST(test_vline);
    RUN_TEST(test_line_diagonal);
    RUN_TEST(test_rect_outline);
    RUN_TEST(test_fill_rect);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `esp-libs/ssd1306/ssd1306_gfx.h`

```c
#ifndef CLIBS_SSD1306_GFX_H
#define CLIBS_SSD1306_GFX_H

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306_fb.h"

/* Graphics primitives over an ssd1306_fb. Every write funnels through
 * ssd1306_fb_set_pixel, which clips out-of-range coords. Pure (no SDK). Not reentrant. */
void ssd1306_draw_hline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, bool on);   /* x..x+w-1 at y */
void ssd1306_draw_vline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t h, bool on);   /* y..y+h-1 at x */
void ssd1306_draw_line(ssd1306_fb *fb, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on); /* integer Bresenham */
void ssd1306_draw_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);     /* outline */
void ssd1306_fill_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);     /* solid */

#endif /* CLIBS_SSD1306_GFX_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_gfx ssd1306/test_ssd1306_gfx.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ssd1306_draw_hline` etc. (ssd1306_fb.c links; ssd1306_gfx.c absent).

- [ ] **Step 4: Write the implementation** — `esp-libs/ssd1306/ssd1306_gfx.c`

```c
#include "ssd1306_gfx.h"

void ssd1306_draw_hline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, bool on) {
    if (!fb) {
        return;
    }
    for (uint8_t i = 0; i < w; i++) {
        ssd1306_fb_set_pixel(fb, (uint8_t)(x + i), y, on);
    }
}

void ssd1306_draw_vline(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t h, bool on) {
    if (!fb) {
        return;
    }
    for (uint8_t i = 0; i < h; i++) {
        ssd1306_fb_set_pixel(fb, x, (uint8_t)(y + i), on);
    }
}

void ssd1306_draw_line(ssd1306_fb *fb, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on) {
    if (!fb) {
        return;
    }
    int dx = (int)x1 - (int)x0; if (dx < 0) dx = -dx;
    int dy = (int)y1 - (int)y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int cx = (int)x0, cy = (int)y0;
    for (;;) {
        ssd1306_fb_set_pixel(fb, (uint8_t)cx, (uint8_t)cy, on);
        if (cx == (int)x1 && cy == (int)y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; cx += sx; }
        if (e2 <  dx) { err += dx; cy += sy; }
    }
}

void ssd1306_draw_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on) {
    if (!fb || w == 0 || h == 0) {
        return;
    }
    ssd1306_draw_hline(fb, x, y, w, on);
    ssd1306_draw_hline(fb, x, (uint8_t)(y + h - 1), w, on);
    ssd1306_draw_vline(fb, x, y, h, on);
    ssd1306_draw_vline(fb, (uint8_t)(x + w - 1), y, h, on);
}

void ssd1306_fill_rect(ssd1306_fb *fb, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on) {
    if (!fb || w == 0 || h == 0) {
        return;
    }
    for (uint8_t i = 0; i < h; i++) {
        ssd1306_draw_hline(fb, x, (uint8_t)(y + i), w, on);
    }
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_gfx ssd1306/test_ssd1306_gfx.c ssd1306/ssd1306_gfx.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/test_ssd1306_gfx`
Expected: PASS — `6 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_gfx.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Issd1306 -o build/san_ssd1306_gfx ssd1306/test_ssd1306_gfx.c ssd1306/ssd1306_gfx.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/san_ssd1306_gfx'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Commit** (pure-only — no glue)

```bash
git commit -m "feat(ssd1306_gfx): line/rect graphics primitives over ssd1306_fb (pure, host-tested)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>" \
  -- esp-libs/ssd1306/ssd1306_gfx.h esp-libs/ssd1306/ssd1306_gfx.c esp-libs/ssd1306/test_ssd1306_gfx.c
```

---

### Task 2: `tm1637` — 4-digit 7-seg (pure + glue)

**Files:** Create `esp-libs/tm1637/tm1637.h`, `tm1637.c`, `tm1637_dev.h`, `tm1637_dev.c`; Test `esp-libs/tm1637/test_tm1637.c`

**Interfaces:**
- Produces: `uint8_t tm1637_digit_segments(uint8_t)`, `void tm1637_encode_number(uint16_t, bool, uint8_t[4])`; glue `tm1637 {gpio_num_t clk,dio; uint8_t brightness}`, `tm1637_init`, `tm1637_show`.

- [ ] **Step 1: Write the failing test** — `esp-libs/tm1637/test_tm1637.c`

```c
#include "unity.h"
#include "tm1637.h"

void setUp(void) {}
void tearDown(void) {}

static void test_digit(void) {
    TEST_ASSERT_EQUAL_HEX8(0x3F, tm1637_digit_segments(0));
    TEST_ASSERT_EQUAL_HEX8(0x7F, tm1637_digit_segments(8));
    TEST_ASSERT_EQUAL_HEX8(0x6F, tm1637_digit_segments(9));
    TEST_ASSERT_EQUAL_HEX8(0x00, tm1637_digit_segments(10));   /* >9 -> blank */
}

static void test_number_full(void) {
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    tm1637_encode_number(1234, false, out);
    uint8_t exp[4] = { 0x06, 0x5B, 0x4F, 0x66 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_number_blank_leading(void) {
    uint8_t out[4] = { 0 };
    tm1637_encode_number(42, false, out);
    uint8_t exp[4] = { 0x00, 0x00, 0x66, 0x5B };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_number_leading_zeros(void) {
    uint8_t out[4] = { 0 };
    tm1637_encode_number(42, true, out);
    uint8_t exp[4] = { 0x3F, 0x3F, 0x66, 0x5B };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_number_zero(void) {
    uint8_t out[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    tm1637_encode_number(0, false, out);
    uint8_t exp[4] = { 0x00, 0x00, 0x00, 0x3F };   /* ones digit always shown */
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_null(void) {
    tm1637_encode_number(1234, false, NULL);   /* no crash */
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_digit);
    RUN_TEST(test_number_full);
    RUN_TEST(test_number_blank_leading);
    RUN_TEST(test_number_leading_zeros);
    RUN_TEST(test_number_zero);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/tm1637/tm1637.h`

```c
#ifndef CLIBS_TM1637_H
#define CLIBS_TM1637_H

#include <stdint.h>
#include <stdbool.h>

/* 7-seg pattern: bit0=a, bit1=b, ... bit6=g, bit7=dp. 0..9 -> pattern; >9 -> 0x00 (blank). */
uint8_t tm1637_digit_segments(uint8_t digit);

/* Render value % 10000 into 4 segment bytes (out[0]=thousands .. out[3]=ones).
 * leading_zeros=false blanks leading zeros (the ones digit is always shown).
 * NULL out -> no-op. */
void tm1637_encode_number(uint16_t value, bool leading_zeros, uint8_t out[4]);

#endif /* CLIBS_TM1637_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Itm1637 -o build/test_tm1637 tm1637/test_tm1637.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_tm1637_digit_segments` / `_tm1637_encode_number`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/tm1637/tm1637.c`

```c
#include "tm1637.h"
#include <stddef.h>

static const uint8_t TM1637_DIGITS[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

uint8_t tm1637_digit_segments(uint8_t digit) {
    return (digit <= 9u) ? TM1637_DIGITS[digit] : 0x00u;
}

void tm1637_encode_number(uint16_t value, bool leading_zeros, uint8_t out[4]) {
    if (out == NULL) {
        return;
    }
    uint16_t v = (uint16_t)(value % 10000u);
    uint8_t d[4];
    d[0] = (uint8_t)((v / 1000u) % 10u);
    d[1] = (uint8_t)((v / 100u) % 10u);
    d[2] = (uint8_t)((v / 10u) % 10u);
    d[3] = (uint8_t)(v % 10u);
    bool seen = leading_zeros;
    for (int i = 0; i < 3; i++) {        /* positions 0..2 may blank; ones (3) always shown */
        if (d[i] != 0) {
            seen = true;
        }
        out[i] = seen ? tm1637_digit_segments(d[i]) : 0x00u;
    }
    out[3] = tm1637_digit_segments(d[3]);
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Itm1637 -o build/test_tm1637 tm1637/test_tm1637.c tm1637/tm1637.c ../common/third_party/unity/unity.c && ./build/test_tm1637`
Expected: PASS — `6 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Itm1637 -c tm1637/tm1637.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Itm1637 -o build/san_tm1637 tm1637/test_tm1637.c tm1637/tm1637.c ../common/third_party/unity/unity.c && ./build/san_tm1637'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/tm1637/tm1637_dev.h` then `tm1637_dev.c`

`tm1637_dev.h`:
```c
#ifndef CLIBS_TM1637_DEV_H
#define CLIBS_TM1637_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "tm1637.h"

/* TM1637 4-digit 7-seg on a 2-wire (CLK/DIO) bus. brightness 0..7. Not reentrant. */
typedef struct { gpio_num_t clk; gpio_num_t dio; uint8_t brightness; } tm1637;

esp_err_t tm1637_init(tm1637 *self, gpio_num_t clk, gpio_num_t dio, uint8_t brightness);
esp_err_t tm1637_show(tm1637 *self, const uint8_t segments[4]);

#endif /* CLIBS_TM1637_DEV_H */
```

`tm1637_dev.c` (not host-compiled; bit-bang is a FIRST CUT — tune on hardware):
```c
#include "tm1637_dev.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

#define TM1637_DELAY_US 5

static void tm1637_start(tm1637 *self) {
    gpio_set_level(self->clk, 1);
    gpio_set_level(self->dio, 1);
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->dio, 0);   /* DIO 1->0 while CLK high = start */
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->clk, 0);
}

static void tm1637_stop(tm1637 *self) {
    gpio_set_level(self->clk, 0);
    gpio_set_level(self->dio, 0);
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->clk, 1);
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->dio, 1);   /* DIO 0->1 while CLK high = stop */
}

static void tm1637_write_byte(tm1637 *self, uint8_t b) {
    for (int i = 0; i < 8; i++) {            /* LSB first */
        gpio_set_level(self->clk, 0);
        gpio_set_level(self->dio, (b >> i) & 1u);
        ets_delay_us(TM1637_DELAY_US);
        gpio_set_level(self->clk, 1);
        ets_delay_us(TM1637_DELAY_US);
    }
    /* ACK clock (DIO released; device pulls low — sample skipped in this first cut) */
    gpio_set_level(self->clk, 0);
    gpio_set_level(self->dio, 1);
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->clk, 1);
    ets_delay_us(TM1637_DELAY_US);
    gpio_set_level(self->clk, 0);
}

esp_err_t tm1637_init(tm1637 *self, gpio_num_t clk, gpio_num_t dio, uint8_t brightness) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->clk = clk;
    self->dio = dio;
    self->brightness = (uint8_t)(brightness & 0x07u);
    gpio_set_direction(clk, GPIO_MODE_OUTPUT);
    gpio_set_direction(dio, GPIO_MODE_OUTPUT);
    gpio_set_level(clk, 1);
    gpio_set_level(dio, 1);
    return ESP_OK;
}

esp_err_t tm1637_show(tm1637 *self, const uint8_t segments[4]) {
    if (!self || !segments) {
        return ESP_ERR_INVALID_ARG;
    }
    tm1637_start(self);                                  /* cmd 1: auto-address write mode */
    tm1637_write_byte(self, 0x40);
    tm1637_stop(self);
    tm1637_start(self);                                  /* cmd 2: start at address 0xC0 */
    tm1637_write_byte(self, 0xC0);
    for (int i = 0; i < 4; i++) {
        tm1637_write_byte(self, segments[i]);
    }
    tm1637_stop(self);
    tm1637_start(self);                                  /* cmd 3: display on + brightness */
    tm1637_write_byte(self, (uint8_t)(0x88u | self->brightness));
    tm1637_stop(self);
    return ESP_OK;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Itm1637 -c tm1637/tm1637.c -o /dev/null && echo OK`  (do NOT host-compile tm1637_dev.c)
```bash
git commit -m "feat(tm1637): 4-digit 7-seg encode (pure, host-tested) + 2-wire bit-bang glue (deferred)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>" \
  -- esp-libs/tm1637/
```

---

### Task 3: `lcd1602` — 16×2 char LCD over PCF8574 (pure + glue)

**Files:** Create `esp-libs/lcd1602/lcd1602.h`, `lcd1602.c`, `lcd1602_dev.h`, `lcd1602_dev.c`; Test `esp-libs/lcd1602/test_lcd1602.c`

**Interfaces:**
- Consumes: `i2c_bus_write` (existing).
- Produces: `void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4])`; glue `lcd1602 {uint8_t addr; bool backlight}`, `lcd1602_init`, `lcd1602_command`, `lcd1602_putc`, `lcd1602_puts`, `lcd1602_set_cursor`.

- [ ] **Step 1: Write the failing test** — `esp-libs/lcd1602/test_lcd1602.c`

```c
#include "unity.h"
#include "lcd1602.h"

void setUp(void) {}
void tearDown(void) {}

/* PCF8574: P0=RS, P1=RW(0), P2=EN, P3=backlight, P4..P7=D4..D7. */
static void test_data_char(void) {
    uint8_t out[4] = { 0 };
    lcd1602_encode_byte(0x41, true, true, out);    /* 'A', RS=1, backlight */
    uint8_t exp[4] = { 0x4D, 0x49, 0x1D, 0x19 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_command(void) {
    uint8_t out[4] = { 0 };
    lcd1602_encode_byte(0x01, false, true, out);   /* clear-display cmd, RS=0, backlight */
    uint8_t exp[4] = { 0x0C, 0x08, 0x1C, 0x18 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_no_backlight_no_rs(void) {
    uint8_t out[4] = { 0 };
    lcd1602_encode_byte(0x41, false, false, out);
    uint8_t exp[4] = { 0x44, 0x40, 0x14, 0x10 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_null(void) {
    lcd1602_encode_byte(0x41, true, true, NULL);   /* no crash */
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_data_char);
    RUN_TEST(test_command);
    RUN_TEST(test_no_backlight_no_rs);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/lcd1602/lcd1602.h`

```c
#ifndef CLIBS_LCD1602_H
#define CLIBS_LCD1602_H

#include <stdint.h>
#include <stdbool.h>

/* Build the 4 PCF8574 byte writes that clock one 8-bit HD44780 value in 4-bit mode.
 * Bit map: P0=RS, P1=RW(0), P2=EN, P3=backlight, P4..P7=D4..D7. Emits
 * hi-nibble|EN, hi-nibble, lo-nibble|EN, lo-nibble (EN high-then-low latches each
 * nibble). NULL out -> no-op. */
void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4]);

#endif /* CLIBS_LCD1602_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ilcd1602 -o build/test_lcd1602 lcd1602/test_lcd1602.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_lcd1602_encode_byte`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/lcd1602/lcd1602.c`

```c
#include "lcd1602.h"
#include <stddef.h>

void lcd1602_encode_byte(uint8_t value, bool rs, bool backlight, uint8_t out[4]) {
    if (out == NULL) {
        return;
    }
    uint8_t hi = (uint8_t)(value & 0xF0u);
    uint8_t lo = (uint8_t)((value << 4) & 0xF0u);   /* value promotes to int; positive, no UB */
    uint8_t base = (uint8_t)((rs ? 0x01u : 0x00u) | (backlight ? 0x08u : 0x00u));
    out[0] = (uint8_t)(hi | base | 0x04u);   /* EN high */
    out[1] = (uint8_t)(hi | base);           /* EN low  */
    out[2] = (uint8_t)(lo | base | 0x04u);
    out[3] = (uint8_t)(lo | base);
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ilcd1602 -o build/test_lcd1602 lcd1602/test_lcd1602.c lcd1602/lcd1602.c ../common/third_party/unity/unity.c && ./build/test_lcd1602`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Ilcd1602 -c lcd1602/lcd1602.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ilcd1602 -o build/san_lcd1602 lcd1602/test_lcd1602.c lcd1602/lcd1602.c ../common/third_party/unity/unity.c && ./build/san_lcd1602'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/lcd1602/lcd1602_dev.h` then `lcd1602_dev.c`

`lcd1602_dev.h`:
```c
#ifndef CLIBS_LCD1602_DEV_H
#define CLIBS_LCD1602_DEV_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lcd1602.h"

/* HD44780 16x2 over a PCF8574 I2C backpack. addr 0x27 (or 0x3F). Not reentrant. */
typedef struct { uint8_t addr; bool backlight; } lcd1602;

esp_err_t lcd1602_init(lcd1602 *self, uint8_t addr);
esp_err_t lcd1602_command(lcd1602 *self, uint8_t cmd);
esp_err_t lcd1602_putc(lcd1602 *self, char c);
esp_err_t lcd1602_puts(lcd1602 *self, const char *s);
esp_err_t lcd1602_set_cursor(lcd1602 *self, uint8_t col, uint8_t row);

#endif /* CLIBS_LCD1602_DEV_H */
```

`lcd1602_dev.c` (not host-compiled; init/EN delays are a FIRST CUT — tune on hardware):
```c
#include "lcd1602_dev.h"
#include "i2c_bus.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

static esp_err_t lcd1602_send(lcd1602 *self, uint8_t value, bool rs) {
    uint8_t seq[4];
    lcd1602_encode_byte(value, rs, self->backlight, seq);
    for (int i = 0; i < 4; i++) {
        esp_err_t err = i2c_bus_write(self->addr, &seq[i], 1);
        if (err != ESP_OK) {
            return err;
        }
        ets_delay_us(50);   /* EN pulse + settle (first cut) */
    }
    return ESP_OK;
}

esp_err_t lcd1602_command(lcd1602 *self, uint8_t cmd) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    return lcd1602_send(self, cmd, false);
}

esp_err_t lcd1602_putc(lcd1602 *self, char c) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    return lcd1602_send(self, (uint8_t)c, true);
}

esp_err_t lcd1602_puts(lcd1602 *self, const char *s) {
    if (!self || !s) {
        return ESP_ERR_INVALID_ARG;
    }
    while (*s) {
        esp_err_t err = lcd1602_putc(self, *s++);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t lcd1602_set_cursor(lcd1602 *self, uint8_t col, uint8_t row) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t addr = (uint8_t)(0x80u | ((row ? 0x40u : 0x00u) + col));
    return lcd1602_command(self, addr);
}

esp_err_t lcd1602_init(lcd1602 *self, uint8_t addr) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    self->backlight = true;
    ets_delay_us(50000);                 /* >40 ms after power-on */
    (void) lcd1602_command(self, 0x33);  /* 4-bit init sequence */
    (void) lcd1602_command(self, 0x32);
    (void) lcd1602_command(self, 0x28);  /* 4-bit, 2-line, 5x8 font */
    (void) lcd1602_command(self, 0x0C);  /* display on, cursor off */
    (void) lcd1602_command(self, 0x06);  /* entry mode: increment */
    esp_err_t err = lcd1602_command(self, 0x01);   /* clear display */
    ets_delay_us(2000);                  /* clear needs >1.5 ms */
    return err;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ilcd1602 -c lcd1602/lcd1602.c -o /dev/null && echo OK`  (do NOT host-compile lcd1602_dev.c)
```bash
git commit -m "feat(lcd1602): PCF8574 4-bit nibble framing (pure, host-tested) + i2c char-LCD glue (deferred)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>" \
  -- esp-libs/lcd1602/
```

---

### Task 4: Integration — Makefile + component.mk + esp-gate + README

**Files:** Modify `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`

**Interfaces:** Consumes Tasks 1–3. Produces `make -C esp-libs test` → 38 suites.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add 3 suites to `.PHONY` + `test:`, a run target each, 3 strict lines.

Add `ssd1306_gfx tm1637 lcd1602` to the `.PHONY` line and to the `test:` aggregate line.

Add these three targets (after the existing `ina219` target, before `strict:`):
```makefile
ssd1306_gfx: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_gfx \
	    ssd1306/test_ssd1306_gfx.c ssd1306/ssd1306_gfx.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_gfx

tm1637: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Itm1637 -o $(BUILD)/test_tm1637 \
	    tm1637/test_tm1637.c tm1637/tm1637.c $(UNITY)/unity.c
	./$(BUILD)/test_tm1637

lcd1602: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ilcd1602 -o $(BUILD)/test_lcd1602 \
	    lcd1602/test_lcd1602.c lcd1602/lcd1602.c $(UNITY)/unity.c
	./$(BUILD)/test_lcd1602
```

Add these three lines to the `strict:` recipe (before its `@echo "strict: OK"`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_gfx.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Itm1637  -c tm1637/tm1637.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ilcd1602 -c lcd1602/lcd1602.c     -o /dev/null
```

- [ ] **Step 2: Run the host gate set**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `38`.
Run: `make -C esp-libs strict`
Expected: `strict: OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `sanitize: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add dirs + objects.

Add `tm1637 lcd1602` to `COMPONENT_ADD_INCLUDEDIRS` and `COMPONENT_SRCDIRS` (`ssd1306` already present). Add to `COMPONENT_OBJS` (with the other `.o` entries):
```makefile
                  ssd1306/ssd1306_gfx.o \
                  tm1637/tm1637.o tm1637/tm1637_dev.o \
                  lcd1602/lcd1602.o lcd1602/lcd1602_dev.o \
```
(`test_*.c` never in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include headers + touch one symbol per lib.

Add near the other includes:
```c
#include "ssd1306_gfx.h"
#include "tm1637_dev.h"
#include "lcd1602_dev.h"
```
Add inside the gate's touch body — the `ssd1306_gfx` touch must come AFTER `oled` is initialized (it reuses the cycle-11 `ssd1306 oled` framebuffer); the others stand alone:
```c
    ssd1306_draw_rect(&oled.fb, 0, 0, 32, 16, true);
    ssd1306_draw_line(&oled.fb, 0, 0, 31, 15, true);

    uint8_t tm_seg[4];
    tm1637 tm;
    tm1637_encode_number(1234, false, tm_seg);
    (void) tm1637_init(&tm, GPIO_NUM_12, GPIO_NUM_13, 7);
    (void) tm1637_show(&tm, tm_seg);

    lcd1602 lcd;
    (void) lcd1602_init(&lcd, 0x27);
    (void) lcd1602_puts(&lcd, "hi");
    ESP_LOGI(TAG, "display/ui linked");
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add rows in the Layer-3 section.

```markdown
### `ssd1306_gfx` — OLED graphics primitives

- `ssd1306_draw_hline` / `_vline` / `_line` (Bresenham) / `_rect` / `_fill_rect`
  (pure, host-tested): draw into an `ssd1306_fb` (writes clip via `set_pixel`).

### `tm1637` — 4-digit 7-seg (2-wire)

- `tm1637_digit_segments` + `tm1637_encode_number` (pure, host-tested). `tm1637_init` /
  `tm1637_show` bit-bang the 2-wire CLK/DIO bus.

### `lcd1602` — 16×2 char LCD (PCF8574 I2C)

- `lcd1602_encode_byte` (pure, host-tested): HD44780 4-bit nibble framing over PCF8574.
  `lcd1602_init` / `lcd1602_command` / `lcd1602_putc` / `lcd1602_puts` / `lcd1602_set_cursor`
  over `i2c_bus`.
```

- [ ] **Step 6: Re-run host gates + (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict && ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `38`, `strict: OK`, `sanitize: OK`.
ESP compile-gate (only if xtensa present; else record DEFERRED):
```bash
# cd esp-gate && make defconfig && make   # -> build/esp_libs_gate.elf (DEFERRED if toolchain absent)
```
```bash
git commit -m "chore(esp-libs): wire ssd1306_gfx/tm1637/lcd1602 into Makefile/component.mk/esp-gate/README (test->38)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>" \
  -- esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
```

---

## Self-Review

**1. Spec coverage:** ssd1306_gfx primitives + vectors → Task 1. tm1637 digit table + encode_number + vectors → Task 2. lcd1602 PCF8574 nibble framing + vectors → Task 3. Makefile→38, component.mk, esp-gate, README → Task 4. All spec sections covered. No Task-0 prereq (gfx pure-on-pure; tm1637 bit-bang; lcd1602 uses existing `i2c_bus_write`).

**2. Placeholder scan:** none — full code in every code step (Bresenham, 7-seg table + leading-zero logic, PCF8574 framing, both glue first-cuts), exact commands + expected output. Commits use pathspec form (matches the patched engine convention).

**3. Type consistency:** pure names (`ssd1306_gfx`,`tm1637`,`lcd1602`) = Makefile targets; component.mk `.o` (`ssd1306/ssd1306_gfx.o`, `tm1637/tm1637.o`+`_dev.o`, `lcd1602/lcd1602.o`+`_dev.o`). `ssd1306_draw_*`, `tm1637_digit_segments`/`tm1637_encode_number`, `lcd1602_encode_byte` identical in header, test, impl, esp-gate. Glue device structs (`tm1637`, `lcd1602`) reuse the bare lib name; the pure headers expose only free functions (no name clash). `ssd1306_gfx` host suite + esp-gate both link `ssd1306_fb.c`. No left-shift of negative (gfx Bresenham uses int; lcd1602 `value<<4` on a positive promoted int; tm1637 table lookup). Vectors recomputed: gfx pixel sets; tm1637 `{06,5B,4F,66}`/`{00,00,66,5B}`/`{3F,3F,66,5B}`/`{00,00,00,3F}`; lcd1602 `{4D,49,1D,19}`/`{0C,08,1C,18}`/`{44,40,14,10}`.
