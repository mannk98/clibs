# esp-libs cycle 12 — L3 stepper + hx711 + rc5 + ssd1306 font/text Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four L3 items to esp-libs — `stepper`, `hx711`, `rc5`, and an `ssd1306` 5×7 text layer — each a host-tested pure part (+ SDK glue, except the pure-only text layer).

**Architecture:** L3 header-split: pure logic (`stepper`/`hx711`/`rc5`/`ssd1306_text`, host-Unity-tested) + SDK glue (`stepper_gpio`/`hx711_gpio`/`rc5_rx`, compile-gate-only). `ssd1306_text` is pure-only (draws into the existing `ssd1306_fb`, no glue). Pure parts run under `make sanitize`.

**Tech Stack:** C11, host `cc` + Unity (`../common/third_party/unity`), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle12-l3-stepper-hx711-rc5-ssd1306font-design.md`

## Global Constraints

- **Branch `feat/esp-libs-cycle12-l3`** (`git checkout -b feat/esp-libs-cycle12-l3` before Task 1).
- **Commands run from `clibs/esp-libs/`.** Unity at `../common/third_party/unity`.
- **Pure files** freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable (`ssd1306_text` also includes `ssd1306_fb.h`, pure).
- **Glue files** SDK (`esp_err.h`, `driver/gpio.h`) → **not host-runnable; COMPILE_GATE deferred** (xtensa absent). `ssd1306_text` has no glue.
- **No UB:** shifts on `uint8`/`uint16`/`uint32` positive operands only.
- **Guards** `CLIBS_<MODULE>_H`; doc comment per public fn/struct; NULL-guard every pointer param; "Not reentrant".
- **Flags:** host `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; sanitize `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (`ASAN_OPTIONS=detect_leaks=0` on darwin).

---

### Task 1: `stepper` — half-step sequence (pure) + GPIO glue (deferred)

**Files:** Create `esp-libs/stepper/stepper.h`, `stepper.c`, `stepper_gpio.h`, `stepper_gpio.c`; Test `esp-libs/stepper/test_stepper.c`

**Interfaces:** Produces `stepper {uint8_t phase; int32_t position}`; `stepper_init`, `stepper_step(stepper*, bool)->uint8_t`, `stepper_coils(const stepper*)->uint8_t`, `stepper_position(const stepper*)->int32_t`; glue `stepper_gpio`, `stepper_gpio_init`, `stepper_gpio_step`.

- [ ] **Step 1: Write the failing test** — `esp-libs/stepper/test_stepper.c`

```c
#include "unity.h"
#include "stepper.h"

void setUp(void) {}
void tearDown(void) {}

static void test_init(void) {
    stepper s;
    stepper_init(&s);
    TEST_ASSERT_EQUAL_HEX8(0x01, stepper_coils(&s));
    TEST_ASSERT_EQUAL_INT32(0, stepper_position(&s));
}

static void test_forward_sequence(void) {
    stepper s;
    stepper_init(&s);
    uint8_t expect[8] = { 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09, 0x01 };
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_HEX8(expect[i], stepper_step(&s, true));
    }
    TEST_ASSERT_EQUAL_INT32(8, stepper_position(&s));
    TEST_ASSERT_EQUAL_HEX8(0x01, stepper_coils(&s));
}

static void test_backward(void) {
    stepper s;
    stepper_init(&s);
    TEST_ASSERT_EQUAL_HEX8(0x09, stepper_step(&s, false));   /* phase 0 -> 7 */
    TEST_ASSERT_EQUAL_INT32(-1, stepper_position(&s));
}

static void test_null(void) {
    TEST_ASSERT_EQUAL_HEX8(0, stepper_step(NULL, true));
    TEST_ASSERT_EQUAL_HEX8(0, stepper_coils(NULL));
    TEST_ASSERT_EQUAL_INT32(0, stepper_position(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_forward_sequence);
    RUN_TEST(test_backward);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/stepper/stepper.h`

```c
#ifndef CLIBS_STEPPER_H
#define CLIBS_STEPPER_H

#include <stdint.h>
#include <stdbool.h>

/* 4-wire unipolar stepper (28BYJ-48) half-step sequencer. Fields private. Not reentrant. */
typedef struct { uint8_t phase; int32_t position; } stepper;

void    stepper_init(stepper *self);                /* phase 0, position 0 */
uint8_t stepper_step(stepper *self, bool forward);  /* advance one half-step; position += dir; returns the new coil pattern (IN1..IN4 = bits 0..3); 0 if NULL */
uint8_t stepper_coils(const stepper *self);         /* current coil pattern, no advance; 0 if NULL */
int32_t stepper_position(const stepper *self);      /* 0 if NULL */

#endif /* CLIBS_STEPPER_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Istepper -o build/test_stepper stepper/test_stepper.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_stepper_init` etc.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/stepper/stepper.c`

```c
#include "stepper.h"

/* 8-phase half-step sequence; coil pattern IN1..IN4 = bits 0..3. */
static const uint8_t STEPPER_HALF[8] = { 0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09 };

void stepper_init(stepper *self) {
    if (!self) {
        return;
    }
    self->phase = 0;
    self->position = 0;
}

uint8_t stepper_step(stepper *self, bool forward) {
    if (!self) {
        return 0;
    }
    self->phase = (uint8_t)((self->phase + (forward ? 1u : 7u)) & 7u);
    self->position += forward ? 1 : -1;
    return STEPPER_HALF[self->phase];
}

uint8_t stepper_coils(const stepper *self) {
    return self ? STEPPER_HALF[self->phase & 7u] : 0;
}

int32_t stepper_position(const stepper *self) {
    return self ? self->position : 0;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Istepper -o build/test_stepper stepper/test_stepper.c stepper/stepper.c ../common/third_party/unity/unity.c && ./build/test_stepper`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Istepper -c stepper/stepper.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Istepper -o build/san_stepper stepper/test_stepper.c stepper/stepper.c ../common/third_party/unity/unity.c && ./build/san_stepper'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/stepper/stepper_gpio.h` then `stepper_gpio.c`

`stepper_gpio.h`:
```c
#ifndef CLIBS_STEPPER_GPIO_H
#define CLIBS_STEPPER_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "stepper.h"

/* 4-wire stepper on four GPIO. Not reentrant. */
typedef struct { stepper drv; gpio_num_t in1, in2, in3, in4; } stepper_gpio;

esp_err_t stepper_gpio_init(stepper_gpio *self, gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4);
esp_err_t stepper_gpio_step(stepper_gpio *self, bool forward);   /* step + drive the 4 coil GPIOs */

#endif /* CLIBS_STEPPER_GPIO_H */
```

`stepper_gpio.c` (not host-compiled):
```c
#include "stepper_gpio.h"

esp_err_t stepper_gpio_init(stepper_gpio *self, gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    stepper_init(&self->drv);
    self->in1 = in1; self->in2 = in2; self->in3 = in3; self->in4 = in4;
    gpio_set_direction(in1, GPIO_MODE_OUTPUT);
    gpio_set_direction(in2, GPIO_MODE_OUTPUT);
    gpio_set_direction(in3, GPIO_MODE_OUTPUT);
    gpio_set_direction(in4, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

esp_err_t stepper_gpio_step(stepper_gpio *self, bool forward) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t p = stepper_step(&self->drv, forward);
    gpio_set_level(self->in1, (p >> 0) & 1u);
    gpio_set_level(self->in2, (p >> 1) & 1u);
    gpio_set_level(self->in3, (p >> 2) & 1u);
    gpio_set_level(self->in4, (p >> 3) & 1u);
    return ESP_OK;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Istepper -c stepper/stepper.c -o /dev/null && echo OK`  (do NOT host-compile stepper_gpio.c)
```bash
git add esp-libs/stepper/
git commit -m "feat(esp-libs/stepper): half-step sequence (pure, host-tested) + GPIO glue (deferred)"
```

---

### Task 2: `hx711` — 24-bit two's-complement parse (pure) + bit-bang glue (deferred)

**Files:** Create `esp-libs/hx711/hx711.h`, `hx711.c`, `hx711_gpio.h`, `hx711_gpio.c`; Test `esp-libs/hx711/test_hx711.c`

**Interfaces:** Produces `hx711_parse(const uint8_t raw[3])->int32_t`; glue `hx711_gain` enum, `hx711_dev`, `hx711_dev_init`, `hx711_dev_read`.

- [ ] **Step 1: Write the failing test** — `esp-libs/hx711/test_hx711.c`

```c
#include "unity.h"
#include "hx711.h"

void setUp(void) {}
void tearDown(void) {}

static void test_parse_values(void) {
    uint8_t zero[3]  = { 0x00, 0x00, 0x00 };
    uint8_t one[3]   = { 0x00, 0x00, 0x01 };
    uint8_t pmax[3]  = { 0x7F, 0xFF, 0xFF };
    uint8_t neg1[3]  = { 0xFF, 0xFF, 0xFF };
    uint8_t nmin[3]  = { 0x80, 0x00, 0x00 };
    TEST_ASSERT_EQUAL_INT32(0,        hx711_parse(zero));
    TEST_ASSERT_EQUAL_INT32(1,        hx711_parse(one));
    TEST_ASSERT_EQUAL_INT32(8388607,  hx711_parse(pmax));   /* 0x7FFFFF */
    TEST_ASSERT_EQUAL_INT32(-1,       hx711_parse(neg1));
    TEST_ASSERT_EQUAL_INT32(-8388608, hx711_parse(nmin));   /* 0x800000 sign-extended */
}

static void test_null(void) {
    TEST_ASSERT_EQUAL_INT32(0, hx711_parse(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_values);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/hx711/hx711.h`

```c
#ifndef CLIBS_HX711_H
#define CLIBS_HX711_H

#include <stdint.h>

/* 24-bit big-endian two's-complement (raw[0]=MSB) -> sign-extended int32. NULL -> 0. */
int32_t hx711_parse(const uint8_t raw[3]);

#endif /* CLIBS_HX711_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ihx711 -o build/test_hx711 hx711/test_hx711.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_hx711_parse`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/hx711/hx711.c`

```c
#include "hx711.h"

int32_t hx711_parse(const uint8_t raw[3]) {
    if (!raw) {
        return 0;
    }
    uint32_t v = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | (uint32_t)raw[2];
    if (v & 0x800000u) {
        v |= 0xFF000000u;   /* sign-extend 24 -> 32 */
    }
    return (int32_t)v;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ihx711 -o build/test_hx711 hx711/test_hx711.c hx711/hx711.c ../common/third_party/unity/unity.c && ./build/test_hx711`
Expected: PASS — `2 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Ihx711 -c hx711/hx711.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ihx711 -o build/san_hx711 hx711/test_hx711.c hx711/hx711.c ../common/third_party/unity/unity.c && ./build/san_hx711'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/hx711/hx711_gpio.h` then `hx711_gpio.c`

`hx711_gpio.h`:
```c
#ifndef CLIBS_HX711_GPIO_H
#define CLIBS_HX711_GPIO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "hx711.h"

/* gain = number of extra SCK pulses after the 24 data bits. */
typedef enum { HX711_GAIN_A128 = 1, HX711_GAIN_B32 = 2, HX711_GAIN_A64 = 3 } hx711_gain;
typedef struct { gpio_num_t sck; gpio_num_t dout; hx711_gain gain; } hx711_dev;

esp_err_t hx711_dev_init(hx711_dev *self, gpio_num_t sck, gpio_num_t dout, hx711_gain gain);
esp_err_t hx711_dev_read(hx711_dev *self, int32_t *out, uint32_t timeout_ms);   /* bit-bang 24 bits + gain pulses, hx711_parse */

#endif /* CLIBS_HX711_GPIO_H */
```

`hx711_gpio.c` (not host-compiled):
```c
#include "hx711_gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

esp_err_t hx711_dev_init(hx711_dev *self, gpio_num_t sck, gpio_num_t dout, hx711_gain gain) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->sck = sck; self->dout = dout; self->gain = gain;
    gpio_set_direction(sck, GPIO_MODE_OUTPUT);
    gpio_set_level(sck, 0);
    gpio_set_direction(dout, GPIO_MODE_INPUT);
    gpio_set_pull_mode(dout, GPIO_PULLUP_ONLY);
    return ESP_OK;
}

esp_err_t hx711_dev_read(hx711_dev *self, int32_t *out, uint32_t timeout_ms) {
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }
    /* DOUT goes low when a sample is ready (bounded wait). */
    uint32_t waited = 0;
    while (gpio_get_level(self->dout)) {
        if (waited++ >= timeout_ms * 1000u) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
    }
    uint8_t raw[3] = { 0, 0, 0 };
    for (int i = 0; i < 24; i++) {                /* 24 data bits, MSB first */
        gpio_set_level(self->sck, 1);
        ets_delay_us(1);
        int b = gpio_get_level(self->dout);
        gpio_set_level(self->sck, 0);
        ets_delay_us(1);
        raw[i / 8] = (uint8_t)((raw[i / 8] << 1) | (b & 1));
    }
    for (int i = 0; i < (int)self->gain; i++) {   /* extra pulses select gain/channel */
        gpio_set_level(self->sck, 1);
        ets_delay_us(1);
        gpio_set_level(self->sck, 0);
        ets_delay_us(1);
    }
    *out = hx711_parse(raw);
    return ESP_OK;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ihx711 -c hx711/hx711.c -o /dev/null && echo OK`  (do NOT host-compile hx711_gpio.c)
```bash
git add esp-libs/hx711/
git commit -m "feat(esp-libs/hx711): 24-bit two's-complement parse (pure, host-tested) + bit-bang glue (deferred)"
```

---

### Task 3: `rc5` — Manchester decode (pure) + GPIO-sample glue (deferred)

**Files:** Create `esp-libs/rc5/rc5.h`, `rc5.c`, `rc5_rx.h`, `rc5_rx.c`; Test `esp-libs/rc5/test_rc5.c`

**Interfaces:** Produces `rc5_result {uint8_t toggle,address,command; uint16_t raw}`; `rc5_decode(const uint8_t *halfbits, size_t count, rc5_result*)->bool`; glue `rc5_rx`, `rc5_rx_init`, `rc5_rx_read`.

- [ ] **Step 1: Write the failing test** — `esp-libs/rc5/test_rc5.c`

```c
#include "unity.h"
#include "rc5.h"

void setUp(void) {}
void tearDown(void) {}

/* Build 28 Manchester half-bits from a 14-bit raw frame, MSB-first: 1 -> (0,1), 0 -> (1,0). */
static void build_rc5(uint16_t raw, uint8_t *hb) {
    for (int i = 0; i < 14; i++) {
        uint8_t bit = (uint8_t)((raw >> (13 - i)) & 1u);
        hb[2 * i]     = bit ? 0 : 1;
        hb[2 * i + 1] = bit ? 1 : 0;
    }
}

static void test_decode_valid(void) {
    uint8_t hb[28];
    build_rc5(0x3535, hb);   /* S1=1 S2=1 T=0 addr=0x14 cmd=0x35 */
    rc5_result r = {0};
    TEST_ASSERT_TRUE(rc5_decode(hb, 28, &r));
    TEST_ASSERT_EQUAL_HEX8(0x14, r.address);
    TEST_ASSERT_EQUAL_HEX8(0x35, r.command);
    TEST_ASSERT_EQUAL_UINT8(0, r.toggle);
    TEST_ASSERT_EQUAL_HEX16(0x3535, r.raw);
}

static void test_malformed_halfbit(void) {
    uint8_t hb[28];
    build_rc5(0x3535, hb);
    hb[0] = 1; hb[1] = 1;   /* (1,1) is not a valid Manchester pair */
    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, 28, &r));
}

static void test_bad_start(void) {
    uint8_t hb[28];
    build_rc5(0x1535, hb);  /* bit13 (S1) == 0 */
    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, 28, &r));
}

static void test_count_and_null(void) {
    uint8_t hb[28];
    build_rc5(0x3535, hb);
    rc5_result r = {0};
    TEST_ASSERT_FALSE(rc5_decode(hb, 10, &r));   /* wrong count */
    TEST_ASSERT_FALSE(rc5_decode(NULL, 28, &r));
    TEST_ASSERT_FALSE(rc5_decode(hb, 28, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_decode_valid);
    RUN_TEST(test_malformed_halfbit);
    RUN_TEST(test_bad_start);
    RUN_TEST(test_count_and_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/rc5/rc5.h`

```c
#ifndef CLIBS_RC5_H
#define CLIBS_RC5_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct { uint8_t toggle; uint8_t address; uint8_t command; uint16_t raw; } rc5_result;

/* Decode RC5 from 28 Manchester half-bit samples (0/1): bit i = (halfbits[2i], halfbits[2i+1]);
 * (0,1) = logical 1, (1,0) = logical 0, anything else -> false. Bits MSB-first into a 14-bit raw
 * (S1 S2 T A4..A0 C5..C0). Validates the leading start bit S1 == 1. address=(raw>>6)&0x1F,
 * command=raw&0x3F, toggle=(raw>>11)&1. Requires count == 28. NULL/bad -> false. Pure, host-testable. */
bool rc5_decode(const uint8_t *halfbits, size_t count, rc5_result *out);

#endif /* CLIBS_RC5_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Irc5 -o build/test_rc5 rc5/test_rc5.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_rc5_decode`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/rc5/rc5.c`

```c
#include "rc5.h"

bool rc5_decode(const uint8_t *halfbits, size_t count, rc5_result *out) {
    if (!halfbits || !out || count != 28) {
        return false;
    }
    uint16_t raw = 0;
    for (size_t i = 0; i < 14; i++) {
        uint8_t h0 = halfbits[2 * i];
        uint8_t h1 = halfbits[2 * i + 1];
        uint8_t bit;
        if (h0 == 0 && h1 == 1) {
            bit = 1;
        } else if (h0 == 1 && h1 == 0) {
            bit = 0;
        } else {
            return false;
        }
        raw = (uint16_t)((raw << 1) | bit);
    }
    if (((raw >> 13) & 1u) != 1u) {        /* start bit S1 must be 1 */
        return false;
    }
    out->raw = raw;
    out->toggle = (uint8_t)((raw >> 11) & 1u);
    out->address = (uint8_t)((raw >> 6) & 0x1Fu);
    out->command = (uint8_t)(raw & 0x3Fu);
    return true;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Irc5 -o build/test_rc5 rc5/test_rc5.c rc5/rc5.c ../common/third_party/unity/unity.c && ./build/test_rc5`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Irc5 -c rc5/rc5.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Irc5 -o build/san_rc5 rc5/test_rc5.c rc5/rc5.c ../common/third_party/unity/unity.c && ./build/san_rc5'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/rc5/rc5_rx.h` then `rc5_rx.c`

`rc5_rx.h`:
```c
#ifndef CLIBS_RC5_RX_H
#define CLIBS_RC5_RX_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "rc5.h"

/* RC5 IR receiver (demodulator output) on one GPIO. Not reentrant. */
typedef struct { gpio_num_t pin; uint8_t halfbits[28]; } rc5_rx;

esp_err_t rc5_rx_init(rc5_rx *self, gpio_num_t pin);
/* Sample the line every ~889us into halfbits[28], then rc5_decode.
 * ESP_OK / ESP_ERR_TIMEOUT (no frame) / ESP_ERR_INVALID_CRC (decode failed) / ESP_ERR_INVALID_ARG. */
esp_err_t rc5_rx_read(rc5_rx *self, rc5_result *out, uint32_t timeout_ms);

#endif /* CLIBS_RC5_RX_H */
```

`rc5_rx.c` (not host-compiled; sampling is a FIRST CUT — tune on hardware):
```c
#include "rc5_rx.h"
#include "esp_timer.h"   /* esp_timer_get_time (us) */
#include "rom/ets_sys.h"

#define RC5_HALFBIT_US 889

esp_err_t rc5_rx_init(rc5_rx *self, gpio_num_t pin) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    return ESP_OK;
}

esp_err_t rc5_rx_read(rc5_rx *self, rc5_result *out, uint32_t timeout_ms) {
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    /* RC5 idles high (demod inverts); wait for the first falling edge (start of the half-bit grid). */
    while (gpio_get_level(self->pin) == 1) {
        if (esp_timer_get_time() > deadline) {
            return ESP_ERR_TIMEOUT;
        }
    }
    /* align to the middle of the first half-bit, then sample 28 half-bits. */
    ets_delay_us(RC5_HALFBIT_US / 2);
    for (int i = 0; i < 28; i++) {
        self->halfbits[i] = (uint8_t)(gpio_get_level(self->pin) ? 1 : 0);
        ets_delay_us(RC5_HALFBIT_US);
    }
    return rc5_decode(self->halfbits, 28, out) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Irc5 -c rc5/rc5.c -o /dev/null && echo OK`  (do NOT host-compile rc5_rx.c)
```bash
git add esp-libs/rc5/
git commit -m "feat(esp-libs/rc5): Manchester RC5 decode (pure, host-tested) + GPIO-sample glue (deferred)"
```

---

### Task 4: `ssd1306_text` — 5×7 font + draw_char/draw_string (pure-only)

**Files:** Create `esp-libs/ssd1306/ssd1306_text.h`, `ssd1306_text.c`; Test `esp-libs/ssd1306/test_ssd1306_text.c`

**Interfaces:**
- Consumes: `ssd1306_fb` (cycle 11): `ssd1306_fb`, `ssd1306_fb_init`, `ssd1306_fb_set_pixel`, `ssd1306_fb_get_pixel`, `SSD1306_FB_BYTES`.
- Produces: `ssd1306_draw_char(ssd1306_fb*, uint8_t x, uint8_t y, char)`, `ssd1306_draw_string(ssd1306_fb*, uint8_t x, uint8_t y, const char*)->uint8_t`.

> Pure-only (draws into `ssd1306_fb`) — no SDK glue, no deferred part.

- [ ] **Step 1: Write the failing test** — `esp-libs/ssd1306/test_ssd1306_text.c`

```c
#include "unity.h"
#include "ssd1306_text.h"
#include "ssd1306_fb.h"

void setUp(void) {}
void tearDown(void) {}

static uint8_t BUF[SSD1306_FB_BYTES(128, 64)];
static const uint8_t FONT_A[5] = { 0x7C, 0x12, 0x11, 0x12, 0x7C };

/* read back a 5x7 cell column (rows 0..6) as a byte, bit0 = top */
static uint8_t col_byte(const ssd1306_fb *fb, uint8_t x) {
    uint8_t b = 0;
    for (uint8_t r = 0; r < 7; r++) {
        if (ssd1306_fb_get_pixel(fb, x, r)) {
            b |= (uint8_t)(1u << r);
        }
    }
    return b;
}

static void test_space_blank(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_char(&fb, 0, 0, ' ');
    for (uint8_t c = 0; c < 5; c++) {
        TEST_ASSERT_EQUAL_HEX8(0x00, col_byte(&fb, c));
    }
}

static void test_char_A(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    ssd1306_draw_char(&fb, 0, 0, 'A');
    for (uint8_t c = 0; c < 5; c++) {
        TEST_ASSERT_EQUAL_HEX8(FONT_A[c], col_byte(&fb, c));
    }
}

static void test_string_advance(void) {
    ssd1306_fb fb;
    ssd1306_fb_init(&fb, BUF, 128, 64);
    TEST_ASSERT_EQUAL_UINT8(6, ssd1306_draw_string(&fb, 0, 0, "A"));
    TEST_ASSERT_EQUAL_UINT8(12, ssd1306_draw_string(&fb, 0, 0, "AA"));
    for (uint8_t c = 0; c < 5; c++) {
        TEST_ASSERT_EQUAL_HEX8(FONT_A[c], col_byte(&fb, (uint8_t)(6 + c)));   /* 2nd 'A' at x=6 */
    }
}

static void test_null(void) {
    ssd1306_draw_char(NULL, 0, 0, 'A');   /* no crash */
    TEST_ASSERT_EQUAL_UINT8(5, ssd1306_draw_string(NULL, 5, 0, "A"));   /* x unchanged */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_space_blank);
    RUN_TEST(test_char_A);
    RUN_TEST(test_string_advance);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `esp-libs/ssd1306/ssd1306_text.h`

```c
#ifndef CLIBS_SSD1306_TEXT_H
#define CLIBS_SSD1306_TEXT_H

#include <stdint.h>
#include "ssd1306_fb.h"

/* 5x7 ASCII text on an ssd1306_fb. Pure (no SDK). Not reentrant. */
void    ssd1306_draw_char(ssd1306_fb *fb, uint8_t x, uint8_t y, char c);   /* unknown char -> blank (space) */
uint8_t ssd1306_draw_string(ssd1306_fb *fb, uint8_t x, uint8_t y, const char *s);  /* 6px/char advance; returns end x */

#endif /* CLIBS_SSD1306_TEXT_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_text ssd1306/test_ssd1306_text.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ssd1306_draw_char` / `_ssd1306_draw_string` (ssd1306_fb.c links, but ssd1306_text.c is absent).

- [ ] **Step 4: Write the implementation** — `esp-libs/ssd1306/ssd1306_text.c`

```c
#include "ssd1306_text.h"

/* Public-domain 5x7 font, ASCII 0x20..0x7E. Column-major: each glyph = 5 column bytes,
 * bit0 = top row (rows 0..6 used). FONT5X7['A'-0x20] == {0x7C,0x12,0x11,0x12,0x7C}. */
static const uint8_t FONT5X7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 0x20 space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x00,0x41,0x22,0x14,0x08}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x01,0x02,0x04,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78}, /* a */
    {0x7F,0x48,0x44,0x44,0x38}, /* b */
    {0x38,0x44,0x44,0x44,0x20}, /* c */
    {0x38,0x44,0x44,0x48,0x7F}, /* d */
    {0x38,0x54,0x54,0x54,0x18}, /* e */
    {0x08,0x7E,0x09,0x01,0x02}, /* f */
    {0x0C,0x52,0x52,0x52,0x3E}, /* g */
    {0x7F,0x08,0x04,0x04,0x78}, /* h */
    {0x00,0x44,0x7D,0x40,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00}, /* j */
    {0x7F,0x10,0x28,0x44,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x08,0x04,0x04,0x78}, /* n */
    {0x38,0x44,0x44,0x44,0x38}, /* o */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x08,0x14,0x14,0x18,0x7C}, /* q */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x54,0x20}, /* s */
    {0x04,0x3F,0x44,0x40,0x20}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* w */
    {0x44,0x28,0x10,0x28,0x44}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* y */
    {0x44,0x64,0x54,0x4C,0x44}, /* z */
    {0x00,0x08,0x36,0x41,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00}, /* } */
    {0x08,0x04,0x08,0x10,0x08}  /* ~ (0x7E) */
};

void ssd1306_draw_char(ssd1306_fb *fb, uint8_t x, uint8_t y, char c) {
    if (!fb) {
        return;
    }
    uint8_t ch = (uint8_t)c;
    if (ch < 0x20u || ch > 0x7Eu) {
        ch = 0x20u;   /* unknown -> space */
    }
    const uint8_t *glyph = FONT5X7[ch - 0x20u];
    for (uint8_t col = 0; col < 5u; col++) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 7u; row++) {
            if (bits & (uint8_t)(1u << row)) {
                ssd1306_fb_set_pixel(fb, (uint8_t)(x + col), (uint8_t)(y + row), true);
            }
        }
    }
}

uint8_t ssd1306_draw_string(ssd1306_fb *fb, uint8_t x, uint8_t y, const char *s) {
    if (!fb || !s) {
        return x;
    }
    while (*s) {
        ssd1306_draw_char(fb, x, y, *s);
        x = (uint8_t)(x + 6u);   /* 5px glyph + 1px gap */
        s++;
    }
    return x;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Issd1306 -o build/test_ssd1306_text ssd1306/test_ssd1306_text.c ssd1306/ssd1306_text.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/test_ssd1306_text`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_text.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Issd1306 -o build/san_ssd1306_text ssd1306/test_ssd1306_text.c ssd1306/ssd1306_text.c ssd1306/ssd1306_fb.c ../common/third_party/unity/unity.c && ./build/san_ssd1306_text'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Commit** (no glue — pure-only)

```bash
git add esp-libs/ssd1306/ssd1306_text.h esp-libs/ssd1306/ssd1306_text.c esp-libs/ssd1306/test_ssd1306_text.c
git commit -m "feat(esp-libs/ssd1306): 5x7 ASCII font + draw_char/draw_string (pure, host-tested)"
```

---

### Task 5: Integration — wire into Makefile + component.mk + esp-gate + README

**Files:** Modify `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`

**Interfaces:** Consumes Tasks 1–4. Produces `make -C esp-libs test` → 29 suites.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add 4 pure suites to `.PHONY` + `test:`, a run target each, 4 strict lines.

Add `stepper hx711 rc5 ssd1306_text` to the `.PHONY` line and the `test:` aggregate line.

Add these four targets (after the existing `rotary` target, before `strict:`):
```makefile
stepper: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Istepper -o $(BUILD)/test_stepper \
	    stepper/test_stepper.c stepper/stepper.c $(UNITY)/unity.c
	./$(BUILD)/test_stepper

hx711: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihx711 -o $(BUILD)/test_hx711 \
	    hx711/test_hx711.c hx711/hx711.c $(UNITY)/unity.c
	./$(BUILD)/test_hx711

rc5: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irc5 -o $(BUILD)/test_rc5 \
	    rc5/test_rc5.c rc5/rc5.c $(UNITY)/unity.c
	./$(BUILD)/test_rc5

ssd1306_text: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_text \
	    ssd1306/test_ssd1306_text.c ssd1306/ssd1306_text.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_text
```

Add these four lines to the `strict:` recipe (before its `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Istepper -c stepper/stepper.c           -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihx711   -c hx711/hx711.c               -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irc5     -c rc5/rc5.c                    -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_text.c       -o /dev/null
```

- [ ] **Step 2: Run the host gate set**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `29`.
Run: `make -C esp-libs strict`
Expected: `strict: OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `sanitize: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add dirs + objects.

Add `stepper hx711 rc5` to `COMPONENT_ADD_INCLUDEDIRS` and `COMPONENT_SRCDIRS` (`ssd1306` already listed). Add to `COMPONENT_OBJS`:
```makefile
                  stepper/stepper.o stepper/stepper_gpio.o \
                  hx711/hx711.o hx711/hx711_gpio.o \
                  rc5/rc5.o rc5/rc5_rx.o \
                  ssd1306/ssd1306_text.o \
```
(Place with the other `.o` entries; `test_*.c` never in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include glue headers + touch one symbol each.

Add near the other includes:
```c
#include "stepper_gpio.h"
#include "hx711_gpio.h"
#include "rc5_rx.h"
#include "ssd1306_text.h"
```
Add inside the gate's touch body:
```c
    stepper_gpio st_dev;
    (void) stepper_gpio_init(&st_dev, 0, 1, 2, 3);

    hx711_dev hx_dev;
    (void) hx711_dev_init(&hx_dev, 14, 12, HX711_GAIN_A128);

    rc5_rx ir5;
    (void) rc5_rx_init(&ir5, 4);

    uint8_t txt_buf[SSD1306_FB_BYTES(128, 64)];
    ssd1306_fb txt_fb;
    ssd1306_fb_init(&txt_fb, txt_buf, 128, 64);
    (void) ssd1306_draw_string(&txt_fb, 0, 0, "hi");
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add rows.

Add to the Layer-3 driver section:
```markdown
### `stepper` — 4-wire unipolar (28BYJ-48)
- `stepper_init` + `stepper_step` (pure, host-tested): half-step coil sequence + position.
  `stepper_gpio_init` / `stepper_gpio_step` drive the 4 coil GPIOs.

### `hx711` — 24-bit load-cell ADC
- `hx711_parse` (pure, host-tested): 24-bit two's-complement → int32. `hx711_dev_init` / `hx711_dev_read`
  bit-bang SCK/DOUT + gain pulses.

### `rc5` — IR RC5 (Manchester)
- `rc5_decode` (pure, host-tested): 28 half-bit samples → address + command. `rc5_rx_init` / `rc5_rx_read` sample + decode.

### `ssd1306` text — 5×7 font
- `ssd1306_draw_char` / `ssd1306_draw_string` (pure, host-tested): render ASCII into an `ssd1306_fb` (6px/char).
```

- [ ] **Step 6: Re-run host gates + (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict && ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `29`, `strict: OK`, `sanitize: OK`.
ESP compile-gate (only if xtensa present; else record DEFERRED):
```bash
# cd esp-gate && make defconfig && make   # -> build/esp_libs_gate.elf (DEFERRED if toolchain absent)
```
```bash
git add esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
git commit -m "chore(esp-libs): wire stepper/hx711/rc5/ssd1306_text into Makefile/component.mk/esp-gate/README (test->29)"
```

---

## Self-Review

**1. Spec coverage:** `stepper` half-step table + step/position → Task 1. `hx711` 24-bit sign-extend vectors → Task 2. `rc5` Manchester decode + `raw=0x3535` + malformed/start/count → Task 3. `ssd1306_text` 5×7 font (`'A'={0x7C,0x12,0x11,0x12,0x7C}`) + draw_char/string + advance → Task 4 (pure-only, no glue). Build infra (Makefile→29, strict, sanitize, component.mk, esp-gate, README) → Task 5. All spec sections covered.

**2. Placeholder scan:** none — full code in every code step (incl. the complete 95-glyph font table); exact commands + expected output; glue is real first-cut code (deferred), not stubs.

**3. Type consistency:** `stepper`/`stepper_gpio`, `hx711_parse`/`hx711_dev`/`hx711_gain`, `rc5_result`/`rc5_rx`, `ssd1306_draw_char`/`ssd1306_draw_string` consistent across header, impl, test, glue, esp-gate touch. Vectors (stepper `0x01..0x09`, hx711 ±, rc5 `0x3535`, font `'A'`) match the spec. Pure-file names (`stepper`,`hx711`,`rc5`,`ssd1306_text`) match the Makefile targets and `component.mk` `.o`. No left-shift of negative (shifts on `uint8`/`uint16`/`uint32`). `ssd1306_text` host target + esp-gate touch correctly link `ssd1306_fb.c`.
