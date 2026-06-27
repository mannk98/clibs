# esp-libs cycle 11 — L3 ssd1306 + max7219 + rotary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three L3 drivers to esp-libs — `ssd1306` (OLED), `max7219` (LED 7-seg/matrix), `rotary` (encoder) — each a host-tested pure part + SDK glue.

**Architecture:** L3 header-split per driver: pure logic (`ssd1306_fb` / `max7219_encode` / `rotary`, host-Unity-tested, no SDK) + SDK glue (`ssd1306` over i2c_bus, `max7219` over spi_bus, `rotary_gpio` over GPIO; compile-gate-only). Pure parts run under `make sanitize`.

**Tech Stack:** C11, host `cc` + Unity (`../common/third_party/unity`), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle11-l3-ssd1306-max7219-rotary-design.md`

## Global Constraints

- **Branch `feat/esp-libs-cycle11-l3`** (`git checkout -b feat/esp-libs-cycle11-l3` before Task 1).
- **Commands run from `clibs/esp-libs/`.** Unity at `../common/third_party/unity`.
- **Pure files** freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable.
- **Glue files** SDK (`esp_err.h`, `i2c_bus.h`/`spi_bus.h`/`driver/gpio.h`) → **not host-runnable; COMPILE_GATE deferred** (xtensa absent).
- **No UB:** shifts on `uint8_t`/`uint32_t` positive operands only (no left-shift of negative — cycle-9 lesson).
- **Guards** `CLIBS_<MODULE>_H`; doc comment per public fn/struct; NULL-guard every pointer param; "Not reentrant".
- **Flags:** host `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; sanitize `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (run with `ASAN_OPTIONS=detect_leaks=0` on darwin).

---

### Task 1: `ssd1306` — 1bpp framebuffer (pure, host-tested) + i2c_bus glue (deferred)

**Files:**
- Create: `esp-libs/ssd1306/ssd1306_fb.h`, `ssd1306_fb.c`, `ssd1306.h`, `ssd1306.c`
- Test: `esp-libs/ssd1306/test_ssd1306_fb.c`

**Interfaces:**
- Produces: `ssd1306_fb {uint8_t *buf, width, height}`; `SSD1306_FB_BYTES(w,h)`; `ssd1306_fb_init/set_pixel/get_pixel/fill/clear`; glue `ssd1306 {uint8_t addr; ssd1306_fb fb}`, `ssd1306_init`, `ssd1306_show`, `SSD1306_ADDR`.

- [ ] **Step 1: Write the failing test** — `esp-libs/ssd1306/test_ssd1306_fb.c`

```c
#include "unity.h"
#include "ssd1306_fb.h"

void setUp(void) {}
void tearDown(void) {}

static uint8_t BUF[SSD1306_FB_BYTES(128, 64)];   /* 1024 bytes */

static void test_set_get_pixel(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 0));
    ssd1306_fb_set_pixel(&fb, 0, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, BUF[0]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 0, 0));
    ssd1306_fb_set_pixel(&fb, 0, 7, true);
    TEST_ASSERT_EQUAL_HEX8(0x81, BUF[0]);          /* bit0 + bit7 */
    ssd1306_fb_set_pixel(&fb, 0, 0, false);
    TEST_ASSERT_EQUAL_HEX8(0x80, BUF[0]);
}

static void test_page_layout(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_fb_set_pixel(&fb, 0, 8, true);         /* page 1, col 0 */
    TEST_ASSERT_EQUAL_HEX8(0x01, BUF[128]);
    ssd1306_fb_set_pixel(&fb, 127, 63, true);      /* last pixel */
    TEST_ASSERT_EQUAL_HEX8(0x80, BUF[1023]);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 127, 63));
}

static void test_fill_clear(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_fb_fill(&fb, true);
    TEST_ASSERT_TRUE(ssd1306_fb_get_pixel(&fb, 50, 50));
    TEST_ASSERT_EQUAL_HEX8(0xFF, BUF[0]);
    ssd1306_fb_clear(&fb);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 50, 50));
    TEST_ASSERT_EQUAL_HEX8(0x00, BUF[0]);
}

static void test_oob_and_null(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);   /* init clears */
    ssd1306_fb_set_pixel(&fb, 128, 0, true);   /* x out of range */
    ssd1306_fb_set_pixel(&fb, 0, 64, true);    /* y out of range */
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 128, 0));
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(&fb, 0, 64));
    TEST_ASSERT_EQUAL_HEX8(0x00, BUF[0]);      /* nothing written */
    ssd1306_fb_set_pixel(NULL, 0, 0, true);
    TEST_ASSERT_FALSE(ssd1306_fb_get_pixel(NULL, 0, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_get_pixel);
    RUN_TEST(test_page_layout);
    RUN_TEST(test_fill_clear);
    RUN_TEST(test_oob_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/ssd1306/ssd1306_fb.h`

```c
#ifndef CLIBS_SSD1306_FB_H
#define CLIBS_SSD1306_FB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 1bpp page-addressed framebuffer for an SSD1306-style OLED. The byte buffer is
 * exactly the SSD1306 GDDRAM layout: idx = (y/8)*width + x, bit = y%8 (bit0 = top
 * row of the page). Fields private. Not reentrant. */
typedef struct { uint8_t *buf; uint8_t width; uint8_t height; } ssd1306_fb;

#define SSD1306_FB_BYTES(w, h) ((size_t)(w) * (h) / 8u)   /* size the caller's buffer */

void ssd1306_fb_init(ssd1306_fb *self, uint8_t *buf, uint8_t width, uint8_t height);  /* clears buf */
void ssd1306_fb_set_pixel(ssd1306_fb *self, uint8_t x, uint8_t y, bool on);           /* OOB/NULL -> no-op */
bool ssd1306_fb_get_pixel(const ssd1306_fb *self, uint8_t x, uint8_t y);              /* OOB/NULL -> false */
void ssd1306_fb_fill(ssd1306_fb *self, bool on);
void ssd1306_fb_clear(ssd1306_fb *self);

#endif /* CLIBS_SSD1306_FB_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_fb ssd1306/test_ssd1306_fb.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ssd1306_fb_init` etc.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/ssd1306/ssd1306_fb.c`

```c
#include "ssd1306_fb.h"

void ssd1306_fb_clear(ssd1306_fb *self) {
    if (!self || !self->buf) {
        return;
    }
    size_t n = SSD1306_FB_BYTES(self->width, self->height);
    for (size_t i = 0; i < n; i++) {
        self->buf[i] = 0;
    }
}

void ssd1306_fb_init(ssd1306_fb *self, uint8_t *buf, uint8_t width, uint8_t height) {
    if (!self) {
        return;
    }
    self->buf = buf;
    self->width = width;
    self->height = height;
    ssd1306_fb_clear(self);
}

void ssd1306_fb_set_pixel(ssd1306_fb *self, uint8_t x, uint8_t y, bool on) {
    if (!self || !self->buf || x >= self->width || y >= self->height) {
        return;
    }
    size_t idx = (size_t)(y >> 3) * self->width + x;
    uint8_t mask = (uint8_t)(1u << (y & 7u));
    if (on) {
        self->buf[idx] |= mask;
    } else {
        self->buf[idx] = (uint8_t)(self->buf[idx] & (uint8_t)~mask);
    }
}

bool ssd1306_fb_get_pixel(const ssd1306_fb *self, uint8_t x, uint8_t y) {
    if (!self || !self->buf || x >= self->width || y >= self->height) {
        return false;
    }
    size_t idx = (size_t)(y >> 3) * self->width + x;
    return ((self->buf[idx] >> (y & 7u)) & 1u) != 0u;
}

void ssd1306_fb_fill(ssd1306_fb *self, bool on) {
    if (!self || !self->buf) {
        return;
    }
    size_t n = SSD1306_FB_BYTES(self->width, self->height);
    uint8_t v = on ? 0xFFu : 0x00u;
    for (size_t i = 0; i < n; i++) {
        self->buf[i] = v;
    }
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_fb ssd1306/test_ssd1306_fb.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/test_ssd1306_fb`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_fb.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Issd1306 -o build/san_ssd1306 ssd1306/test_ssd1306_fb.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/san_ssd1306'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/ssd1306/ssd1306.h` then `ssd1306.c`

`ssd1306.h`:
```c
#ifndef CLIBS_SSD1306_H
#define CLIBS_SSD1306_H

#include <stdint.h>
#include "esp_err.h"
#include "ssd1306_fb.h"

#define SSD1306_ADDR 0x3C

/* SSD1306 OLED over i2c_bus. buf is caller storage >= SSD1306_FB_BYTES(width,height). Not reentrant. */
typedef struct { uint8_t addr; ssd1306_fb fb; } ssd1306;

esp_err_t ssd1306_init(ssd1306 *self, uint8_t addr, uint8_t *buf, uint8_t width, uint8_t height);
esp_err_t ssd1306_show(ssd1306 *self);   /* set addressing window + stream the framebuffer over i2c_bus */

#endif /* CLIBS_SSD1306_H */
```

`ssd1306.c` (not host-compiled):
```c
#include "ssd1306.h"
#include "i2c_bus.h"

/* SSD1306 I2C control byte: 0x00 = command stream, 0x40 = data stream. */
static esp_err_t ssd1306_cmd(uint8_t addr, uint8_t c) {
    return i2c_bus_write_reg(addr, 0x00, &c, 1);
}

esp_err_t ssd1306_init(ssd1306 *self, uint8_t addr, uint8_t *buf, uint8_t width, uint8_t height) {
    if (!self || !buf) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    ssd1306_fb_init(&self->fb, buf, width, height);
    static const uint8_t seq[] = {
        0xAE,             /* display off */
        0x20, 0x00,       /* memory addressing mode = horizontal */
        0x40,             /* display start line 0 */
        0xA1, 0xC8,       /* segment remap + COM scan dir (typical orientation) */
        0x81, 0x7F,       /* contrast */
        0xA4,             /* resume from RAM */
        0xA6,             /* normal (non-inverted) */
        0xD5, 0x80, 0xD3, 0x00, 0xDA, 0x12, 0xD9, 0xF1, 0xDB, 0x40,
        0x8D, 0x14,       /* charge pump on */
        0xAF              /* display on */
    };
    for (size_t i = 0; i < sizeof seq; i++) {
        esp_err_t e = ssd1306_cmd(addr, seq[i]);
        if (e != ESP_OK) {
            return e;
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_show(ssd1306 *self) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t pages = (uint8_t)(self->fb.height / 8u);
    const uint8_t window[] = {
        0x21, 0x00, (uint8_t)(self->fb.width - 1),   /* column address 0..w-1 */
        0x22, 0x00, (uint8_t)(pages - 1)             /* page address 0..pages-1 */
    };
    for (size_t i = 0; i < sizeof window; i++) {
        esp_err_t e = ssd1306_cmd(self->addr, window[i]);
        if (e != ESP_OK) {
            return e;
        }
    }
    return i2c_bus_write_reg(self->addr, 0x40, self->fb.buf,
                            SSD1306_FB_BYTES(self->fb.width, self->fb.height));
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_fb.c -o /dev/null && echo OK`  (do NOT host-compile ssd1306.c)
```bash
git add esp-libs/ssd1306/
git commit -m "feat(esp-libs/ssd1306): 1bpp framebuffer (pure, host-tested) + i2c_bus glue (deferred)"
```

---

### Task 2: `max7219` — frame/segment encode (pure, host-tested) + spi_bus glue (deferred)

**Files:**
- Create: `esp-libs/max7219/max7219_encode.h`, `max7219_encode.c`, `max7219.h`, `max7219.c`
- Test: `esp-libs/max7219/test_max7219_encode.c`

**Interfaces:**
- Produces: `max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2])`, `max7219_digit_segments(uint8_t value, bool dp)->uint8_t`; glue `max7219 {uint8_t intensity}`, `max7219_init`, `max7219_set_digit`, `max7219_set_row`.

- [ ] **Step 1: Write the failing test** — `esp-libs/max7219/test_max7219_encode.c`

```c
#include "unity.h"
#include "max7219_encode.h"

void setUp(void) {}
void tearDown(void) {}

static void test_frame(void) {
    uint8_t out[2] = { 0xAA, 0xAA };
    max7219_frame(0x01, 0x7E, out);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x7E, out[1]);
}

static void test_digit_segments(void) {
    TEST_ASSERT_EQUAL_HEX8(0x7E, max7219_digit_segments(0, false));
    TEST_ASSERT_EQUAL_HEX8(0x30, max7219_digit_segments(1, false));
    TEST_ASSERT_EQUAL_HEX8(0x6D, max7219_digit_segments(2, false));
    TEST_ASSERT_EQUAL_HEX8(0x79, max7219_digit_segments(3, false));
    TEST_ASSERT_EQUAL_HEX8(0x7F, max7219_digit_segments(8, false));
    TEST_ASSERT_EQUAL_HEX8(0xFF, max7219_digit_segments(8, true));   /* + decimal point */
    TEST_ASSERT_EQUAL_HEX8(0x7B, max7219_digit_segments(9, false));
}

static void test_blank(void) {
    TEST_ASSERT_EQUAL_HEX8(0x00, max7219_digit_segments(0x0F, false));   /* out of 0..9 -> blank */
    TEST_ASSERT_EQUAL_HEX8(0x80, max7219_digit_segments(0x0F, true));    /* blank + DP */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_frame);
    RUN_TEST(test_digit_segments);
    RUN_TEST(test_blank);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/max7219/max7219_encode.h`

```c
#ifndef CLIBS_MAX7219_ENCODE_H
#define CLIBS_MAX7219_ENCODE_H

#include <stdint.h>
#include <stdbool.h>

/* Build a 16-bit MAX7219 command (register addr + data) into 2 bytes, MSB-first:
 * out[0]=reg, out[1]=data. NULL out -> no-op. */
void max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2]);

/* Decimal digit 0..9 -> no-decode segment byte (bit7=DP, b6=A, b5=B, b4=C, b3=D,
 * b2=E, b1=F, b0=G). Any other value -> blank (0x00). dp ORs in bit7. */
uint8_t max7219_digit_segments(uint8_t value, bool dp);

#endif /* CLIBS_MAX7219_ENCODE_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Imax7219 -o build/test_max7219_encode max7219/test_max7219_encode.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_max7219_frame` / `_max7219_digit_segments`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/max7219/max7219_encode.c`

```c
#include "max7219_encode.h"

void max7219_frame(uint8_t reg, uint8_t data, uint8_t out[2]) {
    if (!out) {
        return;
    }
    out[0] = reg;
    out[1] = data;
}

uint8_t max7219_digit_segments(uint8_t value, bool dp) {
    static const uint8_t SEG[10] = {
        0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B
    };
    uint8_t s = (value <= 9u) ? SEG[value] : 0x00u;
    if (dp) {
        s |= 0x80u;
    }
    return s;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Imax7219 -o build/test_max7219_encode max7219/test_max7219_encode.c max7219/max7219_encode.c ../common/third_party/unity/unity.c && ./build/test_max7219_encode`
Expected: PASS — `3 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Imax7219 -c max7219/max7219_encode.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Imax7219 -o build/san_max7219 max7219/test_max7219_encode.c max7219/max7219_encode.c ../common/third_party/unity/unity.c && ./build/san_max7219'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/max7219/max7219.h` then `max7219.c`

`max7219.h`:
```c
#ifndef CLIBS_MAX7219_H
#define CLIBS_MAX7219_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "max7219_encode.h"

/* MAX7219 over spi_bus (singleton bus -> no per-instance pin). Not reentrant. */
typedef struct { uint8_t intensity; } max7219;

esp_err_t max7219_init(max7219 *self, uint8_t intensity);             /* 0..15 */
esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp);  /* pos 0..7 */
esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits);  /* 8x8 matrix row 0..7 */

#endif /* CLIBS_MAX7219_H */
```

`max7219.c` (not host-compiled):
```c
#include "max7219.h"
#include "spi_bus.h"

#define MAX7219_REG_DECODE   0x09
#define MAX7219_REG_INTENS   0x0A
#define MAX7219_REG_SCANLIM  0x0B
#define MAX7219_REG_SHUTDOWN 0x0C
#define MAX7219_REG_TEST     0x0F

static esp_err_t max7219_send(uint8_t reg, uint8_t data) {
    uint8_t frame[2];
    max7219_frame(reg, data, frame);
    return spi_bus_transfer(frame, NULL, 2);
}

esp_err_t max7219_init(max7219 *self, uint8_t intensity) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->intensity = intensity;
    esp_err_t e;
    if ((e = max7219_send(MAX7219_REG_TEST, 0x00)) != ESP_OK) {        /* display test off */
        return e;
    }
    if ((e = max7219_send(MAX7219_REG_DECODE, 0x00)) != ESP_OK) {      /* no-decode (raw segments) */
        return e;
    }
    if ((e = max7219_send(MAX7219_REG_SCANLIM, 0x07)) != ESP_OK) {     /* scan all 8 digits */
        return e;
    }
    if ((e = max7219_send(MAX7219_REG_INTENS, (uint8_t)(intensity & 0x0Fu))) != ESP_OK) {
        return e;
    }
    return max7219_send(MAX7219_REG_SHUTDOWN, 0x01);                   /* normal operation */
}

esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp) {
    if (!self || pos > 7u) {
        return ESP_ERR_INVALID_ARG;
    }
    return max7219_send((uint8_t)(0x01u + pos), max7219_digit_segments(value, dp));
}

esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits) {
    if (!self || row > 7u) {
        return ESP_ERR_INVALID_ARG;
    }
    return max7219_send((uint8_t)(0x01u + row), bits);
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Imax7219 -c max7219/max7219_encode.c -o /dev/null && echo OK`  (do NOT host-compile max7219.c)
```bash
git add esp-libs/max7219/
git commit -m "feat(esp-libs/max7219): frame + 7-seg encode (pure, host-tested) + spi_bus glue (deferred)"
```

---

### Task 3: `rotary` — quadrature decode (pure, host-tested) + GPIO glue (deferred)

**Files:**
- Create: `esp-libs/rotary/rotary.h`, `rotary.c`, `rotary_gpio.h`, `rotary_gpio.c`
- Test: `esp-libs/rotary/test_rotary.c`

**Interfaces:**
- Produces: `rotary_dir {ROTARY_NONE,ROTARY_CW,ROTARY_CCW}`; `rotary {uint8_t prev_ab; int32_t position}`; `rotary_init`, `rotary_update(rotary*, uint8_t ab)->rotary_dir`, `rotary_position(const rotary*)->int32_t`; glue `rotary_gpio {rotary enc; gpio_num_t pin_a, pin_b}`, `rotary_gpio_init`, `rotary_gpio_poll`.

- [ ] **Step 1: Write the failing test** — `esp-libs/rotary/test_rotary.c`

```c
#include "unity.h"
#include "rotary.h"

void setUp(void) {}
void tearDown(void) {}

static void test_cw(void) {
    rotary r;
    rotary_init(&r, 0);
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&r, 1));  /* 00 -> 01 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&r, 3));  /* 01 -> 11 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&r, 2));  /* 11 -> 10 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&r, 0));  /* 10 -> 00 */
    TEST_ASSERT_EQUAL_INT32(4, rotary_position(&r));
}

static void test_ccw(void) {
    rotary r;
    rotary_init(&r, 0);
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&r, 2));  /* 00 -> 10 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&r, 3));  /* 10 -> 11 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&r, 1));  /* 11 -> 01 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&r, 0));  /* 01 -> 00 */
    TEST_ASSERT_EQUAL_INT32(-4, rotary_position(&r));
}

static void test_invalid_and_null(void) {
    rotary r;
    rotary_init(&r, 0);
    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(&r, 3));  /* 00 -> 11 (2-step jump) */
    TEST_ASSERT_EQUAL_INT32(0, rotary_position(&r));
    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(&r, 3));  /* 11 -> 11 (no change) */
    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(NULL, 1));
    TEST_ASSERT_EQUAL_INT32(0, rotary_position(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cw);
    RUN_TEST(test_ccw);
    RUN_TEST(test_invalid_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/rotary/rotary.h`

```c
#ifndef CLIBS_ROTARY_H
#define CLIBS_ROTARY_H

#include <stdint.h>

typedef enum { ROTARY_NONE, ROTARY_CW, ROTARY_CCW } rotary_dir;

/* Quadrature rotary-encoder decoder (quarter-step). Fields private. Not reentrant. */
typedef struct { uint8_t prev_ab; int32_t position; } rotary;

void       rotary_init(rotary *self, uint8_t ab);   /* ab = (a<<1)|b, current A/B level */
/* Feed the new (a<<1)|b. A valid Gray transition counts +1 (CW) / -1 (CCW) into position and returns
 * ROTARY_CW/ROTARY_CCW; same state or a 2-step jump returns ROTARY_NONE (position unchanged). NULL self
 * -> ROTARY_NONE. */
rotary_dir rotary_update(rotary *self, uint8_t ab);
int32_t    rotary_position(const rotary *self);     /* 0 if NULL */

#endif /* CLIBS_ROTARY_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Irotary -o build/test_rotary rotary/test_rotary.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_rotary_init` / `_rotary_update`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/rotary/rotary.c`

```c
#include "rotary.h"

/* Quarter-step Gray transition table, indexed by (prev_ab<<2)|ab. +1 = CW, -1 = CCW, 0 = none/illegal. */
static const int8_t ROTARY_TABLE[16] = {
    0, 1, -1, 0,  -1, 0, 0, 1,  1, 0, 0, -1,  0, -1, 1, 0
};

void rotary_init(rotary *self, uint8_t ab) {
    if (!self) {
        return;
    }
    self->prev_ab = (uint8_t)(ab & 0x3u);
    self->position = 0;
}

rotary_dir rotary_update(rotary *self, uint8_t ab) {
    if (!self) {
        return ROTARY_NONE;
    }
    ab = (uint8_t)(ab & 0x3u);
    int8_t d = ROTARY_TABLE[(uint8_t)((self->prev_ab << 2) | ab)];
    self->prev_ab = ab;
    if (d > 0) {
        self->position += 1;
        return ROTARY_CW;
    }
    if (d < 0) {
        self->position -= 1;
        return ROTARY_CCW;
    }
    return ROTARY_NONE;
}

int32_t rotary_position(const rotary *self) {
    return self ? self->position : 0;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Irotary -o build/test_rotary rotary/test_rotary.c rotary/rotary.c ../common/third_party/unity/unity.c && ./build/test_rotary`
Expected: PASS — `3 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Irotary -c rotary/rotary.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Irotary -o build/san_rotary rotary/test_rotary.c rotary/rotary.c ../common/third_party/unity/unity.c && ./build/san_rotary'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/rotary/rotary_gpio.h` then `rotary_gpio.c`

`rotary_gpio.h`:
```c
#ifndef CLIBS_ROTARY_GPIO_H
#define CLIBS_ROTARY_GPIO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "rotary.h"

/* Quadrature encoder on two GPIO (A/B). Not reentrant. */
typedef struct { rotary enc; gpio_num_t pin_a; gpio_num_t pin_b; } rotary_gpio;

esp_err_t  rotary_gpio_init(rotary_gpio *self, gpio_num_t pin_a, gpio_num_t pin_b);
rotary_dir rotary_gpio_poll(rotary_gpio *self);   /* read A/B -> (a<<1)|b -> rotary_update */

#endif /* CLIBS_ROTARY_GPIO_H */
```

`rotary_gpio.c` (not host-compiled):
```c
#include "rotary_gpio.h"

static uint8_t rotary_gpio_read(gpio_num_t a, gpio_num_t b) {
    return (uint8_t)(((gpio_get_level(a) != 0) << 1) | (gpio_get_level(b) != 0));
}

esp_err_t rotary_gpio_init(rotary_gpio *self, gpio_num_t pin_a, gpio_num_t pin_b) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin_a = pin_a;
    self->pin_b = pin_b;
    gpio_set_direction(pin_a, GPIO_MODE_INPUT);
    gpio_set_direction(pin_b, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin_a, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pin_b, GPIO_PULLUP_ONLY);
    rotary_init(&self->enc, rotary_gpio_read(pin_a, pin_b));
    return ESP_OK;
}

rotary_dir rotary_gpio_poll(rotary_gpio *self) {
    if (!self) {
        return ROTARY_NONE;
    }
    return rotary_update(&self->enc, rotary_gpio_read(self->pin_a, self->pin_b));
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Irotary -c rotary/rotary.c -o /dev/null && echo OK`  (do NOT host-compile rotary_gpio.c)
```bash
git add esp-libs/rotary/
git commit -m "feat(esp-libs/rotary): quadrature decode (pure, host-tested) + GPIO glue (deferred)"
```

---

### Task 4: Integration — wire into Makefile + component.mk + esp-gate + README

**Files:**
- Modify: `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`

**Interfaces:**
- Consumes: Tasks 1–3.
- Produces: `make -C esp-libs test` → 25 suites; component build includes the 3 drivers; esp-gate touches one symbol each.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add the 3 pure suites to `.PHONY` + `test:`, a run target each, and 3 strict lines.

Add `ssd1306_fb max7219_encode rotary` to the `.PHONY` line and the `test:` aggregate line.

Add these three targets (after the existing `pir` target, before `strict:`):
```makefile
ssd1306_fb: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_fb \
	    ssd1306/test_ssd1306_fb.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_fb

max7219_encode: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imax7219 -o $(BUILD)/test_max7219_encode \
	    max7219/test_max7219_encode.c max7219/max7219_encode.c $(UNITY)/unity.c
	./$(BUILD)/test_max7219_encode

rotary: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irotary -o $(BUILD)/test_rotary \
	    rotary/test_rotary.c rotary/rotary.c $(UNITY)/unity.c
	./$(BUILD)/test_rotary
```

Add these three lines to the `strict:` recipe (before its `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_fb.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imax7219 -c max7219/max7219_encode.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irotary  -c rotary/rotary.c            -o /dev/null
```

- [ ] **Step 2: Run the host gate set**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `25`.
Run: `make -C esp-libs strict`
Expected: `strict: OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `sanitize: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add dirs + objects.

Add `ssd1306 max7219 rotary` to `COMPONENT_ADD_INCLUDEDIRS` and `COMPONENT_SRCDIRS`. Add to `COMPONENT_OBJS`:
```makefile
                  ssd1306/ssd1306_fb.o ssd1306/ssd1306.o \
                  max7219/max7219_encode.o max7219/max7219.o \
                  rotary/rotary.o rotary/rotary_gpio.o \
```
(Place with the other `.o` entries; `test_*.c` never in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include the 3 glue headers + touch one symbol each.

Add near the other includes:
```c
#include "ssd1306.h"
#include "max7219.h"
#include "rotary_gpio.h"
```
Add inside the gate's touch body:
```c
    uint8_t oled_buf[SSD1306_FB_BYTES(128, 64)];
    ssd1306 oled;
    (void) ssd1306_init(&oled, SSD1306_ADDR, oled_buf, 128, 64);

    uint8_t mx_frame[2];
    max7219 mx;
    (void) max7219_frame(0x01, max7219_digit_segments(8, true), mx_frame);
    (void) max7219_init(&mx, 8);

    rotary_gpio knob;
    (void) rotary_gpio_init(&knob, 12, 13);
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add 3 L3 rows.

Add to the Layer-3 driver section:
```markdown
### `ssd1306` — 128×64 OLED (I2C)
- `ssd1306_fb_*` (pure, host-tested): 1bpp page framebuffer (set/get/fill/clear pixel).
  `ssd1306_init` + `ssd1306_show` drive the panel over `i2c_bus`.

### `max7219` — LED 7-seg / 8×8 matrix (SPI)
- `max7219_frame` + `max7219_digit_segments` (pure, host-tested). `max7219_init` /
  `max7219_set_digit` / `max7219_set_row` over `spi_bus`.

### `rotary` — quadrature encoder
- `rotary_init` + `rotary_update` (pure, host-tested): table-driven quarter-step decode → CW/CCW + position.
  `rotary_gpio_init` / `rotary_gpio_poll` read the A/B pins.
```

- [ ] **Step 6: Re-run host gates + (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict && ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `25`, `strict: OK`, `sanitize: OK`.
ESP compile-gate (only if xtensa present; else record DEFERRED):
```bash
# cd esp-gate && make defconfig && make   # -> build/esp_libs_gate.elf (DEFERRED if toolchain absent)
```
```bash
git add esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
git commit -m "chore(esp-libs): wire ssd1306/max7219/rotary into Makefile/component.mk/esp-gate/README (test->25)"
```

---

## Self-Review

**1. Spec coverage:** `ssd1306_fb` page math with the `buf[0]=0x01`/`buf[128]`/`buf[1023]=0x80` vectors → Task 1. `max7219` frame + segment table (`0x7E…0x7B`, DP) → Task 2. `rotary` quarter-step table + ±4 sequences + 2-step-jump NONE → Task 3. Each glue (deferred) → Tasks 1–3 step 6. Build infra (Makefile→25, strict, sanitize, component.mk, esp-gate, README) → Task 4. All spec sections covered.

**2. Placeholder scan:** none — full code in every code step; exact commands + expected output; glue is real code (deferred), not stubs.

**3. Type consistency:** `ssd1306_fb`/`ssd1306`, `max7219`, `rotary`/`rotary_dir`/`rotary_gpio` and all signatures identical across header, impl, test, glue, esp-gate touch. Vectors (`buf[0]=0x01`, segment `0x7E/0x30/0x6D/0x79/0x7F/0xFF/0x7B`, rotary ±4) match the spec. Pure-file names (`ssd1306_fb`, `max7219_encode`, `rotary`) match the Makefile targets and `component.mk` 6 `.o`. No left-shift of negative (shifts on `uint8_t`/`uint32_t`, table is `int8_t` lookup not shift).
