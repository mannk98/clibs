# esp-libs cycle 10 — L3 ws2812 + ir_nec + pir Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three L3 drivers to esp-libs — `ws2812` (RGB), `ir_nec` (IR receive), `pir` (motion) — each with a host-tested pure part + SDK glue.

**Architecture:** L3 header-split per driver: a pure logic file (`ws2812_render` / `ir_nec` / `pir`, host-Unity-tested, no SDK) + SDK glue (`ws2812` / `ir_nec_rx` / `pir_gpio`, compile-gate-only). Pure parts run under `make sanitize` (UBSan+ASan).

**Tech Stack:** C11, host `cc` + Unity (`../common/third_party/unity`), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle10-l3-ws2812-ir-pir-design.md`

## Global Constraints

- **Branch `feat/esp-libs-cycle10-l3`** (`git checkout -b feat/esp-libs-cycle10-l3` before Task 1).
- **Commands run from `clibs/esp-libs/`.** Unity at `../common/third_party/unity`.
- **Pure files** freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable.
- **Glue files** SDK (`esp_err.h`, `driver/gpio.h`, timing) → **not host-runnable; COMPILE_GATE deferred** (xtensa absent).
- **No UB:** no left-shift of a possibly-negative value (the batch-1/cycle-9 lesson); shifts here are on `uint32_t`/positive only.
- **Guards** `CLIBS_<MODULE>_H`; doc comment per public fn/struct; NULL-guard every pointer param; "Not reentrant".
- **Flags:** host `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; sanitize `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (run with `ASAN_OPTIONS=detect_leaks=0` on darwin).

---

### Task 1: `ws2812` — RGB render (pure, host-tested) + bit-bang glue (deferred)

**Files:**
- Create: `esp-libs/ws2812/ws2812_render.h`, `esp-libs/ws2812/ws2812_render.c`, `esp-libs/ws2812/ws2812.h`, `esp-libs/ws2812/ws2812.c`
- Test: `esp-libs/ws2812/test_ws2812_render.c`

**Interfaces:**
- Produces: `ws2812_rgb {uint8_t r,g,b}`; `ws2812_render(const ws2812_rgb*, size_t n, uint8_t brightness, uint8_t *out, size_t out_size)->size_t`; glue `ws2812 {gpio_num_t pin; uint16_t count; uint8_t *grb; size_t grb_size}`, `ws2812_init(...)`, `ws2812_show(...)`.

- [ ] **Step 1: Write the failing test** — `esp-libs/ws2812/test_ws2812_render.c`

```c
#include "unity.h"
#include "ws2812_render.h"

void setUp(void) {}
void tearDown(void) {}

static void test_full_brightness_grb(void) {
    ws2812_rgb px = { .r = 255, .g = 128, .b = 0 };
    uint8_t out[3] = {0};
    TEST_ASSERT_EQUAL_UINT(3, ws2812_render(&px, 1, 255, out, sizeof out));
    TEST_ASSERT_EQUAL_UINT8(128, out[0]);  /* G */
    TEST_ASSERT_EQUAL_UINT8(255, out[1]);  /* R */
    TEST_ASSERT_EQUAL_UINT8(0,   out[2]);  /* B */
}

static void test_half_brightness(void) {
    ws2812_rgb px = { .r = 255, .g = 128, .b = 0 };
    uint8_t out[3] = {0};
    ws2812_render(&px, 1, 128, out, sizeof out);
    TEST_ASSERT_EQUAL_UINT8(64,  out[0]);  /* 128*128/255 */
    TEST_ASSERT_EQUAL_UINT8(128, out[1]);  /* 255*128/255 */
    TEST_ASSERT_EQUAL_UINT8(0,   out[2]);
}

static void test_zero_brightness(void) {
    ws2812_rgb px = { .r = 255, .g = 255, .b = 255 };
    uint8_t out[3] = {9, 9, 9};
    ws2812_render(&px, 1, 0, out, sizeof out);
    TEST_ASSERT_EQUAL_UINT8(0, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0, out[2]);
}

static void test_two_pixels(void) {
    ws2812_rgb px[2] = { { .r = 255, .g = 0, .b = 0 }, { .r = 0, .g = 0, .b = 255 } };
    uint8_t out[6] = {0};
    TEST_ASSERT_EQUAL_UINT(6, ws2812_render(px, 2, 255, out, sizeof out));
    TEST_ASSERT_EQUAL_UINT8(0,   out[0]); TEST_ASSERT_EQUAL_UINT8(255, out[1]); TEST_ASSERT_EQUAL_UINT8(0,   out[2]);
    TEST_ASSERT_EQUAL_UINT8(0,   out[3]); TEST_ASSERT_EQUAL_UINT8(0,   out[4]); TEST_ASSERT_EQUAL_UINT8(255, out[5]);
}

static void test_guards(void) {
    ws2812_rgb px = { .r = 1, .g = 2, .b = 3 };
    uint8_t out[3];
    TEST_ASSERT_EQUAL_UINT(0, ws2812_render(&px, 1, 255, out, 2));   /* out too small */
    TEST_ASSERT_EQUAL_UINT(0, ws2812_render(&px, 0, 255, out, 3));   /* n 0 */
    TEST_ASSERT_EQUAL_UINT(0, ws2812_render(NULL, 1, 255, out, 3));
    TEST_ASSERT_EQUAL_UINT(0, ws2812_render(&px, 1, 255, NULL, 3));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_brightness_grb);
    RUN_TEST(test_half_brightness);
    RUN_TEST(test_zero_brightness);
    RUN_TEST(test_two_pixels);
    RUN_TEST(test_guards);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/ws2812/ws2812_render.h`

```c
#ifndef CLIBS_WS2812_RENDER_H
#define CLIBS_WS2812_RENDER_H

#include <stdint.h>
#include <stddef.h>

/* One pixel, as the caller supplies it (R, G, B). */
typedef struct { uint8_t r, g, b; } ws2812_rgb;

/* Render n pixels into a WS2812 GRB byte buffer: out[3i]=G, out[3i+1]=R,
 * out[3i+2]=B, each channel scaled by brightness/255. out_size must be >= 3*n.
 * Returns 3*n, or 0 if out_size<3*n / NULL px or out / n==0. Pure, host-testable. */
size_t ws2812_render(const ws2812_rgb *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size);

#endif /* CLIBS_WS2812_RENDER_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Iws2812 -o build/test_ws2812_render ws2812/test_ws2812_render.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ws2812_render`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/ws2812/ws2812_render.c`

```c
#include "ws2812_render.h"

/* scale a channel by brightness/255; (uint16 product <= 65025, no overflow/UB) */
static uint8_t scale(uint8_t c, uint8_t brightness) {
    return (uint8_t)(((uint16_t)c * (uint16_t)brightness) / 255u);
}

size_t ws2812_render(const ws2812_rgb *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size) {
    if (!px || !out || n == 0) {
        return 0;
    }
    size_t need = n * 3u;
    if (out_size < need) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        out[i * 3 + 0] = scale(px[i].g, brightness);   /* G */
        out[i * 3 + 1] = scale(px[i].r, brightness);   /* R */
        out[i * 3 + 2] = scale(px[i].b, brightness);   /* B */
    }
    return need;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Iws2812 -o build/test_ws2812_render ws2812/test_ws2812_render.c ws2812/ws2812_render.c ../common/third_party/unity/unity.c && ./build/test_ws2812_render`
Expected: PASS — `5 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Iws2812 -c ws2812/ws2812_render.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Iws2812 -o build/san_ws2812 ws2812/test_ws2812_render.c ws2812/ws2812_render.c ../common/third_party/unity/unity.c && ./build/san_ws2812'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/ws2812/ws2812.h` then `esp-libs/ws2812/ws2812.c`

`ws2812.h`:
```c
#ifndef CLIBS_WS2812_H
#define CLIBS_WS2812_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ws2812_render.h"

/* WS2812 strip on one GPIO. grb_buf is caller storage >= 3*count bytes (no malloc). Not reentrant. */
typedef struct { gpio_num_t pin; uint16_t count; uint8_t *grb; size_t grb_size; } ws2812;

esp_err_t ws2812_init(ws2812 *self, gpio_num_t pin, uint16_t count, uint8_t *grb_buf, size_t grb_size);
/* Render px (count pixels) at brightness into grb, then bit-bang it out the pin. */
esp_err_t ws2812_show(ws2812 *self, const ws2812_rgb *px, uint8_t brightness);

#endif /* CLIBS_WS2812_H */
```

`ws2812.c` (timing-critical bit-bang — FIRST CUT, tune on hardware; not host-compiled):
```c
#include "ws2812.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* WS2812 @800kHz: T0H~0.35us/T0L~0.8us, T1H~0.7us/T1L~0.6us, reset >50us. ets_delay_us
 * is too coarse for the sub-us highs — this is a FIRST CUT structure; on hardware
 * replace the per-bit highs/lows with a cycle-counted (xthal_get_ccount) inline routine
 * with interrupts disabled. Verified only by the deferred esp-gate link. */
esp_err_t ws2812_init(ws2812 *self, gpio_num_t pin, uint16_t count, uint8_t *grb_buf, size_t grb_size) {
    if (!self || !grb_buf) {
        return ESP_ERR_INVALID_ARG;
    }
    if (grb_size < (size_t)count * 3u) {
        return ESP_ERR_INVALID_SIZE;
    }
    self->pin = pin;
    self->count = count;
    self->grb = grb_buf;
    self->grb_size = grb_size;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    return ESP_OK;
}

static void ws2812_send_byte(gpio_num_t pin, uint8_t b) {
    for (int i = 7; i >= 0; i--) {        /* MSB first */
        if (b & (1u << i)) {
            gpio_set_level(pin, 1); ets_delay_us(1); gpio_set_level(pin, 0);
        } else {
            gpio_set_level(pin, 1); gpio_set_level(pin, 0); ets_delay_us(1);
        }
    }
}

esp_err_t ws2812_show(ws2812 *self, const ws2812_rgb *px, uint8_t brightness) {
    if (!self || !px) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t n = ws2812_render(px, self->count, brightness, self->grb, self->grb_size);
    if (n == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    for (size_t i = 0; i < n; i++) {
        ws2812_send_byte(self->pin, self->grb[i]);
    }
    ets_delay_us(60);                     /* latch/reset */
    return ESP_OK;
}
```

- [ ] **Step 7: Confirm pure part still builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Iws2812 -c ws2812/ws2812_render.c -o /dev/null && echo OK`  (do NOT host-compile ws2812.c — SDK)
```bash
git add esp-libs/ws2812/
git commit -m "feat(esp-libs/ws2812): RGB render (pure, host-tested) + bit-bang glue (deferred)"
```

---

### Task 2: `ir_nec` — NEC decode (pure, host-tested) + capture glue (deferred)

**Files:**
- Create: `esp-libs/ir_nec/ir_nec.h`, `esp-libs/ir_nec/ir_nec.c`, `esp-libs/ir_nec/ir_nec_rx.h`, `esp-libs/ir_nec/ir_nec_rx.c`
- Test: `esp-libs/ir_nec/test_ir_nec.c`

**Interfaces:**
- Produces: `ir_nec_result {uint8_t address, command; uint32_t raw}`; `ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result*)->bool`; glue `ir_nec_rx {gpio_num_t pin; uint16_t buf[70]}`, `ir_nec_rx_init`, `ir_nec_rx_read`.

- [ ] **Step 1: Write the failing test** — `esp-libs/ir_nec/test_ir_nec.c`

```c
#include "unity.h"
#include "ir_nec.h"

void setUp(void) {}
void tearDown(void) {}

/* Build a 66-entry NEC duration array for addr/cmd with the standard
 * addr/~addr/cmd/~cmd byte order, LSB-first bits. */
static void build_nec(uint8_t addr, uint8_t cmd, uint16_t *us) {
    uint8_t bytes[4] = { addr, (uint8_t)~addr, cmd, (uint8_t)~cmd };
    us[0] = 9000;   /* leading mark */
    us[1] = 4500;   /* leading space */
    int idx = 2;
    for (int b = 0; b < 4; b++) {
        for (int bit = 0; bit < 8; bit++) {
            us[idx++] = 562;                                       /* mark */
            us[idx++] = (bytes[b] & (1u << bit)) ? 1687 : 562;     /* space: 1 / 0 */
        }
    }
}

static void test_decode_valid(void) {
    uint16_t us[66];
    build_nec(0x00, 0x16, us);
    ir_nec_result r = {0};
    TEST_ASSERT_TRUE(ir_nec_decode(us, 66, &r));
    TEST_ASSERT_EQUAL_HEX8(0x00, r.address);
    TEST_ASSERT_EQUAL_HEX8(0x16, r.command);
    TEST_ASSERT_EQUAL_HEX32(0xE916FF00u, r.raw);
}

static void test_bad_complement(void) {
    uint16_t us[66];
    build_nec(0x00, 0x16, us);
    us[65] = (us[65] == 562) ? 1687 : 562;   /* flip ~cmd bit7 -> complement fails */
    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, 66, &r));
}

static void test_bad_header(void) {
    uint16_t us[66];
    build_nec(0x12, 0x34, us);
    us[0] = 3000;   /* leading mark out of +/-25% tolerance */
    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, 66, &r));
}

static void test_short_and_null(void) {
    uint16_t us[66];
    build_nec(0x12, 0x34, us);
    ir_nec_result r = {0};
    TEST_ASSERT_FALSE(ir_nec_decode(us, 10, &r));     /* count < 66 */
    TEST_ASSERT_FALSE(ir_nec_decode(NULL, 66, &r));
    TEST_ASSERT_FALSE(ir_nec_decode(us, 66, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_decode_valid);
    RUN_TEST(test_bad_complement);
    RUN_TEST(test_bad_header);
    RUN_TEST(test_short_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/ir_nec/ir_nec.h`

```c
#ifndef CLIBS_IR_NEC_H
#define CLIBS_IR_NEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct { uint8_t address; uint8_t command; uint32_t raw; } ir_nec_result;

/* Decode a NEC frame from mark/space durations in microseconds: us[0]=leading mark
 * (~9000), us[1]=leading space (~4500), then 32 (mark, space) pairs. Requires
 * count >= 66. Header tolerance +/-25% (mark 6750..11250, space 3375..5625). Each
 * data space is 0 (~562) or 1 (~1687) by the 1124us midpoint; marks are ignored.
 * Bits are assembled LSB-first into `raw`; address = raw & 0xFF, command =
 * (raw>>16)&0xFF; the command complement ((raw>>24)&0xFF == (uint8_t)~command) is
 * validated. Returns true + fills out on a valid frame; false on bad header /
 * count<66 / failed complement / NULL. Pure, host-testable. */
bool ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result *out);

#endif /* CLIBS_IR_NEC_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Iir_nec -o build/test_ir_nec ir_nec/test_ir_nec.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ir_nec_decode`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/ir_nec/ir_nec.c`

```c
#include "ir_nec.h"

bool ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result *out) {
    if (!us || !out || count < 66) {
        return false;
    }
    /* leading 9ms mark + 4.5ms space, +/-25% */
    if (us[0] < 6750u || us[0] > 11250u) {
        return false;
    }
    if (us[1] < 3375u || us[1] > 5625u) {
        return false;
    }
    uint32_t raw = 0;
    for (uint8_t i = 0; i < 32u; i++) {
        uint16_t space = us[3u + 2u * i];          /* data space of bit i (mark us[2+2i] ignored) */
        if (space > 1124u) {                        /* 562 <-> 1687 midpoint */
            raw |= (uint32_t)1u << i;               /* LSB-first; shift of positive, no UB */
        }
    }
    uint8_t address = (uint8_t)(raw & 0xFFu);
    uint8_t command = (uint8_t)((raw >> 16) & 0xFFu);
    uint8_t cmd_inv = (uint8_t)((raw >> 24) & 0xFFu);
    if (cmd_inv != (uint8_t)~command) {             /* command complement must hold */
        return false;
    }
    out->address = address;
    out->command = command;
    out->raw = raw;
    return true;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Iir_nec -o build/test_ir_nec ir_nec/test_ir_nec.c ir_nec/ir_nec.c ../common/third_party/unity/unity.c && ./build/test_ir_nec`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Iir_nec -c ir_nec/ir_nec.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Iir_nec -o build/san_ir_nec ir_nec/test_ir_nec.c ir_nec/ir_nec.c ../common/third_party/unity/unity.c && ./build/san_ir_nec'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/ir_nec/ir_nec_rx.h` then `ir_nec_rx.c`

`ir_nec_rx.h`:
```c
#ifndef CLIBS_IR_NEC_RX_H
#define CLIBS_IR_NEC_RX_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ir_nec.h"

/* IR receiver (active-low demodulator output, e.g. VS1838B) on one GPIO. Not reentrant. */
typedef struct { gpio_num_t pin; uint16_t buf[70]; } ir_nec_rx;

esp_err_t ir_nec_rx_init(ir_nec_rx *self, gpio_num_t pin);
/* Capture one frame's mark/space durations (bounded by timeout_ms), then ir_nec_decode.
 * ESP_OK / ESP_ERR_TIMEOUT (no frame) / ESP_ERR_INVALID_CRC (decode failed) / ESP_ERR_INVALID_ARG. */
esp_err_t ir_nec_rx_read(ir_nec_rx *self, ir_nec_result *out, uint32_t timeout_ms);

#endif /* CLIBS_IR_NEC_RX_H */
```

`ir_nec_rx.c` (edge-timing capture — FIRST CUT, tune on hardware; not host-compiled):
```c
#include "ir_nec_rx.h"
#include "esp_timer.h"   /* esp_timer_get_time (us) */
#include "rom/ets_sys.h"

/* Poll the demodulator pin (idle high, active low) and record durations between
 * edges into buf, until an inter-edge gap exceeds the NEC frame end (~20ms) or the
 * timeout elapses. Polling capture is a FIRST CUT — on hardware prefer a GPIO edge
 * ISR timestamping into buf. Verified only by the deferred esp-gate link. */
esp_err_t ir_nec_rx_init(ir_nec_rx *self, gpio_num_t pin) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);   /* idle high */
    return ESP_OK;
}

esp_err_t ir_nec_rx_read(ir_nec_rx *self, ir_nec_result *out, uint32_t timeout_ms) {
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    /* wait for the falling edge that starts the leading mark */
    while (gpio_get_level(self->pin) == 1) {
        if (esp_timer_get_time() > deadline) {
            return ESP_ERR_TIMEOUT;
        }
    }
    size_t n = 0;
    int last = 0;                                  /* current level (just went low) */
    int64_t t_prev = esp_timer_get_time();
    while (n < (sizeof self->buf / sizeof self->buf[0])) {
        int level = gpio_get_level(self->pin);
        int64_t now = esp_timer_get_time();
        if (level != last) {
            int64_t dur = now - t_prev;
            self->buf[n++] = (uint16_t)(dur > 65535 ? 65535 : dur);
            t_prev = now;
            last = level;
        } else if (now - t_prev > 20000) {         /* end of frame (long idle) */
            break;
        }
        if (now > deadline) {
            break;
        }
    }
    if (n < 66) {
        return ESP_ERR_TIMEOUT;
    }
    return ir_nec_decode(self->buf, n, out) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
```

- [ ] **Step 7: Confirm pure part builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Iir_nec -c ir_nec/ir_nec.c -o /dev/null && echo OK`  (do NOT host-compile ir_nec_rx.c)
```bash
git add esp-libs/ir_nec/
git commit -m "feat(esp-libs/ir_nec): NEC decode (pure, host-tested) + GPIO capture glue (deferred)"
```

---

### Task 3: `pir` — motion FSM (pure, host-tested) + GPIO glue (deferred)

**Files:**
- Create: `esp-libs/pir/pir.h`, `esp-libs/pir/pir.c`, `esp-libs/pir/pir_gpio.h`, `esp-libs/pir/pir_gpio.c`
- Test: `esp-libs/pir/test_pir.c`

**Interfaces:**
- Produces: `pir_event {PIR_NONE,PIR_MOTION_START,PIR_MOTION_END}`; `pir {uint32_t hold_ms,last_ms; bool active}`; `pir_init`, `pir_update(pir*, bool, uint32_t)->pir_event`, `pir_active(const pir*)->bool`; glue `pir_gpio {pir fsm; gpio_num_t pin; bool active_high}`, `pir_gpio_init`, `pir_gpio_poll`.

- [ ] **Step 1: Write the failing test** — `esp-libs/pir/test_pir.c`

```c
#include "unity.h"
#include "pir.h"

void setUp(void) {}
void tearDown(void) {}

static void test_motion_sequence(void) {
    pir p;
    pir_init(&p, 1000);
    TEST_ASSERT_FALSE(pir_active(&p));
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_START, pir_update(&p, true, 0));
    TEST_ASSERT_TRUE(pir_active(&p));
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 500));    /* within hold */
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, true, 800));     /* re-trigger, last=800 */
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 1700));   /* 900 < 1000 */
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_END, pir_update(&p, false, 1900)); /* 1100 >= 1000 */
    TEST_ASSERT_FALSE(pir_active(&p));
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 2000));   /* idle stays idle */
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_START, pir_update(&p, true, 2100));
}

static void test_null(void) {
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(NULL, true, 0));
    TEST_ASSERT_FALSE(pir_active(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_motion_sequence);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/pir/pir.h`

```c
#ifndef CLIBS_PIR_H
#define CLIBS_PIR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum { PIR_NONE, PIR_MOTION_START, PIR_MOTION_END } pir_event;

/* Software retrigger/hold for a PIR sensor. Fields private. Not reentrant. */
typedef struct { uint32_t hold_ms; uint32_t last_ms; bool active; } pir;

void pir_init(pir *self, uint32_t hold_ms);
/* Feed the raw sensor level + a monotonic now_ms. motion_raw refreshes last_ms; on
 * the rising edge from idle returns PIR_MOTION_START. With no motion and active and
 * (now-last) >= hold_ms returns PIR_MOTION_END (and goes idle). Else PIR_NONE.
 * NULL self -> PIR_NONE. now compared via wrap-safe unsigned subtraction. */
pir_event pir_update(pir *self, bool motion_raw, uint32_t now_ms);
bool pir_active(const pir *self);   /* false if NULL */

#endif /* CLIBS_PIR_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ipir -o build/test_pir pir/test_pir.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_pir_init` / `_pir_update`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/pir/pir.c`

```c
#include "pir.h"

void pir_init(pir *self, uint32_t hold_ms) {
    if (!self) {
        return;
    }
    self->hold_ms = hold_ms;
    self->last_ms = 0;
    self->active = false;
}

pir_event pir_update(pir *self, bool motion_raw, uint32_t now_ms) {
    if (!self) {
        return PIR_NONE;
    }
    if (motion_raw) {
        self->last_ms = now_ms;
        if (!self->active) {
            self->active = true;
            return PIR_MOTION_START;
        }
        return PIR_NONE;
    }
    if (self->active && (uint32_t)(now_ms - self->last_ms) >= self->hold_ms) {
        self->active = false;
        return PIR_MOTION_END;
    }
    return PIR_NONE;
}

bool pir_active(const pir *self) {
    return self ? self->active : false;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ipir -o build/test_pir pir/test_pir.c pir/pir.c ../common/third_party/unity/unity.c && ./build/test_pir`
Expected: PASS — `2 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Ipir -c pir/pir.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ipir -o build/san_pir pir/test_pir.c pir/pir.c ../common/third_party/unity/unity.c && ./build/san_pir'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/pir/pir_gpio.h` then `pir_gpio.c`

`pir_gpio.h`:
```c
#ifndef CLIBS_PIR_GPIO_H
#define CLIBS_PIR_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "pir.h"

/* PIR sensor on one GPIO + the software hold FSM. Not reentrant. */
typedef struct { pir fsm; gpio_num_t pin; bool active_high; } pir_gpio;

esp_err_t pir_gpio_init(pir_gpio *self, gpio_num_t pin, uint32_t hold_ms, bool active_high);
/* Read the pin and run the FSM with the caller's monotonic now_ms. */
pir_event pir_gpio_poll(pir_gpio *self, uint32_t now_ms);

#endif /* CLIBS_PIR_GPIO_H */
```

`pir_gpio.c` (not host-compiled):
```c
#include "pir_gpio.h"

esp_err_t pir_gpio_init(pir_gpio *self, gpio_num_t pin, uint32_t hold_ms, bool active_high) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    pir_init(&self->fsm, hold_ms);
    self->pin = pin;
    self->active_high = active_high;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    /* ESP8266 has no internal pull-down: an active-high PIR (push-pull output) needs
     * no pull; an active-low wiring uses the internal pull-up. */
    gpio_set_pull_mode(pin, active_high ? GPIO_FLOATING : GPIO_PULLUP_ONLY);
    return ESP_OK;
}

pir_event pir_gpio_poll(pir_gpio *self, uint32_t now_ms) {
    if (!self) {
        return PIR_NONE;
    }
    int level = gpio_get_level(self->pin);
    bool motion = self->active_high ? (level != 0) : (level == 0);
    return pir_update(&self->fsm, motion, now_ms);
}
```

- [ ] **Step 7: Confirm pure part builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ipir -c pir/pir.c -o /dev/null && echo OK`  (do NOT host-compile pir_gpio.c)
```bash
git add esp-libs/pir/
git commit -m "feat(esp-libs/pir): motion retrigger/hold FSM (pure, host-tested) + GPIO glue (deferred)"
```

---

### Task 4: Integration — wire into Makefile + component.mk + esp-gate + README

**Files:**
- Modify: `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`

**Interfaces:**
- Consumes: Tasks 1–3.
- Produces: `make -C esp-libs test` → 22 suites; component build includes the 3 drivers; esp-gate touches one symbol each.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add the 3 pure suites to `.PHONY` + `test:`, a run target each, and 3 strict lines.

Add `ws2812_render ir_nec pir` to the `.PHONY` line and to the `test:` aggregate line.

Add these three targets (after the existing `bme280_parse` target, before `strict:`):
```makefile
ws2812_render: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iws2812 -o $(BUILD)/test_ws2812_render \
	    ws2812/test_ws2812_render.c ws2812/ws2812_render.c $(UNITY)/unity.c
	./$(BUILD)/test_ws2812_render

ir_nec: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iir_nec -o $(BUILD)/test_ir_nec \
	    ir_nec/test_ir_nec.c ir_nec/ir_nec.c $(UNITY)/unity.c
	./$(BUILD)/test_ir_nec

pir: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ipir -o $(BUILD)/test_pir \
	    pir/test_pir.c pir/pir.c $(UNITY)/unity.c
	./$(BUILD)/test_pir
```

Add these three lines to the `strict:` recipe (before its `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Iws2812 -c ws2812/ws2812_render.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iir_nec -c ir_nec/ir_nec.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ipir    -c pir/pir.c               -o /dev/null
```

- [ ] **Step 2: Run the host gate set**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `22`.
Run: `make -C esp-libs strict`
Expected: `strict: OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `sanitize: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add the dirs + objects.

Add `ws2812 ir_nec pir` to `COMPONENT_ADD_INCLUDEDIRS` and to `COMPONENT_SRCDIRS`. Add to `COMPONENT_OBJS`:
```makefile
                  ws2812/ws2812_render.o ws2812/ws2812.o \
                  ir_nec/ir_nec.o ir_nec/ir_nec_rx.o \
                  pir/pir.o pir/pir_gpio.o \
```
(Place with the other `.o` entries; `test_*.c` are never in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include the 3 glue headers + touch one symbol each.

Add near the other includes:
```c
#include "ws2812.h"
#include "ir_nec_rx.h"
#include "pir_gpio.h"
```
Add inside the gate's touch body (links pure + glue symbols without hardware):
```c
    ws2812_rgb ws_px = {0};
    uint8_t ws_buf[3];
    ws2812 ws_dev;
    (void) ws2812_render(&ws_px, 1, 255, ws_buf, sizeof ws_buf);
    (void) ws2812_init(&ws_dev, 2, 1, ws_buf, sizeof ws_buf);

    uint16_t ir_us[66] = {0};
    ir_nec_result ir_r;
    ir_nec_rx ir_dev;
    (void) ir_nec_decode(ir_us, 66, &ir_r);
    (void) ir_nec_rx_init(&ir_dev, 4);

    pir_gpio pir_dev;
    (void) pir_gpio_init(&pir_dev, 5, 1000, true);
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add 3 L3 rows.

Add to the Layer-3 driver section:
```markdown
### `ws2812` — WS2812/NeoPixel RGB
- `ws2812_render(px, n, brightness, out, out_size)` (pure, host-tested): RGB → GRB byte
  buffer with brightness scaling. `ws2812_init` + `ws2812_show` bit-bang the strip (first cut).

### `ir_nec` — IR remote receive (NEC)
- `ir_nec_decode(us, count, &result)` (pure, host-tested): mark/space durations → address +
  command (command complement validated; `raw` for extended remotes). `ir_nec_rx_init/_read` capture + decode.

### `pir` — motion sensor
- `pir_init` + `pir_update(raw, now_ms)` (pure, host-tested): retrigger/hold FSM → PIR_MOTION_START/END.
  `pir_gpio_init/_poll` read the pin + run the FSM.
```

- [ ] **Step 6: Re-run host gates + (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict && ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `22`, `strict: OK`, `sanitize: OK`.
ESP compile-gate (only if xtensa toolchain present; else record DEFERRED):
```bash
# cd esp-gate && make defconfig && make   # -> build/esp_libs_gate.elf (DEFERRED if toolchain absent)
```
```bash
git add esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
git commit -m "chore(esp-libs): wire ws2812/ir_nec/pir into Makefile/component.mk/esp-gate/README (test->22)"
```

---

## Self-Review

**1. Spec coverage:** `ws2812_render` GRB+brightness with the `{128,255,0}`/`{64,128,0}` vectors → Task 1. `ir_nec_decode` midpoint/header-tol/LSB-first/cmd-complement with `raw=0xE916FF00` → Task 2. `pir` START/END FSM worked sequence → Task 3. Each glue (deferred) → Tasks 1–3 step 6. Build infra (Makefile→22, strict, sanitize, component.mk, esp-gate, README) → Task 4. All spec sections covered.

**2. Placeholder scan:** none — full code in every code step; exact commands + expected output; glue marked first-cut/deferred with real code (not stubs).

**3. Type consistency:** `ws2812_rgb`/`ws2812`, `ir_nec_result`/`ir_nec_rx`, `pir`/`pir_event`/`pir_gpio` and all signatures identical across header, impl, test, glue, esp-gate touch. Vectors (`{128,255,0}`, `0xE916FF00`, the pir sequence) match the spec. Pure-file names (`ws2812_render`, `ir_nec`, `pir`) match the Makefile targets and `component.mk` `.o` list (6 objects). No left-shift of negative anywhere (raw is `uint32_t`, shifts of positive constants).
