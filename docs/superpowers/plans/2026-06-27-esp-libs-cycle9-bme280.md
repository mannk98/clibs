# esp-libs cycle 9 — bme280 L3 driver Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a BME280 (T/H/P over I²C) L3 driver to esp-libs — pure host-tested Bosch compensation + calibration parsing, plus SDK glue over `i2c_bus`.

**Architecture:** L3 header-split: `bme280_parse.{h,c}` (pure: `bme280_parse_calib` + `bme280_compensate`, host-Unity-tested) and `bme280.{h,c}` (SDK glue: forced-mode read over `i2c_bus`, compile-gate only). SI output units (milli-°C / milli-%RH / Pa).

**Tech Stack:** C11, host `cc` + Unity (`esp-libs/third_party/unity`... actually `../common/third_party/unity` — see commands), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle9-bme280-design.md`

## Global Constraints

- **Work on branch `feat/esp-libs-cycle9-bme280`** (`git checkout -b feat/esp-libs-cycle9-bme280` before Task 1). Merge/push is the user's call.
- **Commands run from `clibs/esp-libs/`** unless stated. Unity is at `../common/third_party/unity` (esp-libs reuses common's Unity — see the esp-libs Makefile).
- **Pure part** (`bme280_parse.{h,c}`): freestanding `<stdint.h>`/`<stdbool.h>` only, no SDK, no malloc, host-testable.
- **Glue** (`bme280.{h,c}`): SDK headers (`esp_err.h`, `i2c_bus.h`, FreeRTOS) → **not host-runnable; COMPILE_GATE deferred** (xtensa toolchain absent).
- **No UB:** never left-shift a possibly-negative value (C11 6.5.7p4 — the batch-1 `fixed` lesson). The Bosch formulas' `<<` on signed values are rewritten as multiplication by the equivalent power of two (identical value, well-defined for negatives). Right-shifts of signed values are implementation-defined (not UB) and kept.
- **Guards** `CLIBS_BME280_PARSE_H` / `CLIBS_BME280_H`; doc comment per public function/struct; NULL-guard every pointer param; "Not reentrant".
- **Flags:** host build `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; UBSan pass `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (esp-libs has no `sanitize` make target — run the command directly).

---

### Task 1: `bme280_parse` — calibration parse + Bosch compensation (pure, host-tested)

**Files:**
- Create: `esp-libs/bme280/bme280_parse.h`
- Create: `esp-libs/bme280/bme280_parse.c`
- Test: `esp-libs/bme280/test_bme280_parse.c`

**Interfaces:**
- Consumes: nothing (pure).
- Produces: `bme280_calib` struct, `bme280_reading` struct, `bme280_parse_calib(const uint8_t[26], const uint8_t[7], bme280_calib*)->bool`, `bme280_compensate(const bme280_calib*, int32_t adc_T, int32_t adc_P, int32_t adc_H, bme280_reading*)->bool`.

- [ ] **Step 1: Write the failing test** — `esp-libs/bme280/test_bme280_parse.c`

```c
#include "unity.h"
#include "bme280_parse.h"

void setUp(void) {}
void tearDown(void) {}

/* Calib register blocks (0x88..0xA1, 0xE1..0xE7) — cross-checked vectors. */
static const uint8_t CALIB26[26] = {
    0x45,0x6F,0x6F,0x68,0x32,0x00,0xF4,0x93,0x43,0xD6,0xD0,0x0B,0xBC,0x18,
    0xEB,0xFF,0xF9,0xFF,0x8C,0x3C,0xF8,0xC6,0x70,0x17,0x00,0x4B};
static const uint8_t CALIBH[7] = {0x72,0x01,0x00,0x12,0x02,0x00,0x1E};

static void test_parse_calib_fields(void) {
    bme280_calib c;
    TEST_ASSERT_TRUE(bme280_parse_calib(CALIB26, CALIBH, &c));
    TEST_ASSERT_EQUAL_UINT16(28485, c.T1);
    TEST_ASSERT_EQUAL_INT16(26735, c.T2);
    TEST_ASSERT_EQUAL_INT16(50, c.T3);
    TEST_ASSERT_EQUAL_UINT16(37876, c.P1);
    TEST_ASSERT_EQUAL_INT16(-10685, c.P2);
    TEST_ASSERT_EQUAL_INT16(3024, c.P3);
    TEST_ASSERT_EQUAL_INT16(6332, c.P4);
    TEST_ASSERT_EQUAL_INT16(-21, c.P5);
    TEST_ASSERT_EQUAL_INT16(-7, c.P6);
    TEST_ASSERT_EQUAL_INT16(15500, c.P7);
    TEST_ASSERT_EQUAL_INT16(-14600, c.P8);
    TEST_ASSERT_EQUAL_INT16(6000, c.P9);
    TEST_ASSERT_EQUAL_UINT8(75, c.H1);
    TEST_ASSERT_EQUAL_INT16(370, c.H2);
    TEST_ASSERT_EQUAL_UINT8(0, c.H3);
    TEST_ASSERT_EQUAL_INT16(290, c.H4);
    TEST_ASSERT_EQUAL_INT16(0, c.H5);
    TEST_ASSERT_EQUAL_INT8(30, c.H6);
}

static void test_parse_calib_h4h5_sign_extension(void) {
    /* calibH[3..5] = {0xFC,0xCE,0xF9} -> H4 = -50, H5 = -100 (12-bit signed) */
    uint8_t ch[7] = {0x72,0x01,0x00, 0xFC,0xCE,0xF9, 0x1E};
    bme280_calib c;
    TEST_ASSERT_TRUE(bme280_parse_calib(CALIB26, ch, &c));
    TEST_ASSERT_EQUAL_INT16(-50, c.H4);
    TEST_ASSERT_EQUAL_INT16(-100, c.H5);
}

static void test_compensate_si(void) {
    bme280_calib c;
    bme280_parse_calib(CALIB26, CALIBH, &c);
    bme280_reading r;
    TEST_ASSERT_TRUE(bme280_compensate(&c, 519888, 326816, 26083, &r));
    TEST_ASSERT_EQUAL_INT32(20440, r.temp_mc);     /* 20.44 C */
    TEST_ASSERT_EQUAL_UINT32(101576, r.press_pa);  /* 1015.76 hPa */
    TEST_ASSERT_EQUAL_UINT32(42743, r.humi_mrh);   /* 42.743 %RH */
}

static void test_null_guards(void) {
    bme280_calib c; bme280_reading r;
    TEST_ASSERT_FALSE(bme280_parse_calib(NULL, CALIBH, &c));
    TEST_ASSERT_FALSE(bme280_parse_calib(CALIB26, NULL, &c));
    TEST_ASSERT_FALSE(bme280_parse_calib(CALIB26, CALIBH, NULL));
    TEST_ASSERT_FALSE(bme280_compensate(NULL, 1, 1, 1, &r));
    TEST_ASSERT_FALSE(bme280_compensate(&c, 1, 1, 1, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_calib_fields);
    RUN_TEST(test_parse_calib_h4h5_sign_extension);
    RUN_TEST(test_compensate_si);
    RUN_TEST(test_null_guards);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the header** — `esp-libs/bme280/bme280_parse.h`

```c
#ifndef CLIBS_BME280_PARSE_H
#define CLIBS_BME280_PARSE_H

#include <stdint.h>
#include <stdbool.h>

/* Bosch BME280 trimming coefficients (fields private; populate via bme280_parse_calib). */
typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} bme280_calib;

/* Compensated readings in SI-friendly units. */
typedef struct {
    int32_t  temp_mc;    /* temperature, milli-degrees C (20440 = 20.44 C) */
    uint32_t humi_mrh;   /* relative humidity, milli-%RH (42743 = 42.743 %RH) */
    uint32_t press_pa;   /* pressure, pascals (101576 = 1015.76 hPa) */
} bme280_reading;

/* Parse the calib register blocks: calib26 = regs 0x88..0xA1 (26 bytes),
 * calibH = regs 0xE1..0xE7 (7 bytes). Little-endian u16/s16; H4/H5 are packed
 * 12-bit signed:
 *   H4 = sign12((calibH[3] << 4) | (calibH[4] & 0x0F));
 *   H5 = sign12((calibH[5] << 4) | (calibH[4] >> 4));
 * Any NULL arg -> false. Pure (no SDK), host-testable. */
bool bme280_parse_calib(const uint8_t calib26[26], const uint8_t calibH[7], bme280_calib *out);

/* Bosch integer compensation (datasheet 4.2.3): T int32, P int64, H int32,
 * converted to SI: temp_mc = T(0.01C)*10; press_pa = P(Q24.8)>>8;
 * humi_mrh = (H(Q22.10)*1000)>>10. adc_T/adc_P are 20-bit, adc_H 16-bit.
 * NULL cal/out -> false. Pure, host-testable, no UB. */
bool bme280_compensate(const bme280_calib *cal, int32_t adc_T, int32_t adc_P, int32_t adc_H, bme280_reading *out);

#endif /* CLIBS_BME280_PARSE_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ibme280 -o build/test_bme280_parse bme280/test_bme280_parse.c ../common/third_party/unity/unity.c`
Expected: FAIL — link error: undefined symbols `_bme280_parse_calib`, `_bme280_compensate` (no `bme280_parse.c` yet).

- [ ] **Step 4: Write the implementation** — `esp-libs/bme280/bme280_parse.c`

```c
#include "bme280_parse.h"

/* sign-extend a 12-bit two's-complement value to int16_t */
static int16_t sign12(int32_t v) { return (int16_t)(v >= 2048 ? v - 4096 : v); }

bool bme280_parse_calib(const uint8_t calib26[26], const uint8_t calibH[7], bme280_calib *out)
{
    if (!calib26 || !calibH || !out) {
        return false;
    }
    out->T1 = (uint16_t)(calib26[0]  | (calib26[1]  << 8));
    out->T2 = (int16_t) (calib26[2]  | (calib26[3]  << 8));
    out->T3 = (int16_t) (calib26[4]  | (calib26[5]  << 8));
    out->P1 = (uint16_t)(calib26[6]  | (calib26[7]  << 8));
    out->P2 = (int16_t) (calib26[8]  | (calib26[9]  << 8));
    out->P3 = (int16_t) (calib26[10] | (calib26[11] << 8));
    out->P4 = (int16_t) (calib26[12] | (calib26[13] << 8));
    out->P5 = (int16_t) (calib26[14] | (calib26[15] << 8));
    out->P6 = (int16_t) (calib26[16] | (calib26[17] << 8));
    out->P7 = (int16_t) (calib26[18] | (calib26[19] << 8));
    out->P8 = (int16_t) (calib26[20] | (calib26[21] << 8));
    out->P9 = (int16_t) (calib26[22] | (calib26[23] << 8));
    /* calib26[24] = reg 0xA0 (reserved, skipped) */
    out->H1 = calib26[25];
    out->H2 = (int16_t)(calibH[0] | (calibH[1] << 8));
    out->H3 = calibH[2];
    out->H4 = sign12(((int32_t)calibH[3] << 4) | (calibH[4] & 0x0F));
    out->H5 = sign12(((int32_t)calibH[5] << 4) | ((calibH[4] >> 4) & 0x0F));
    out->H6 = (int8_t)calibH[6];
    return true;
}

bool bme280_compensate(const bme280_calib *cal, int32_t adc_T, int32_t adc_P, int32_t adc_H, bme280_reading *out)
{
    if (!cal || !out) {
        return false;
    }

    /* --- temperature: Bosch int32 -> t_fine, T in 0.01 C --- */
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)cal->T1 << 1))) * (int32_t)cal->T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)cal->T1) * ((adc_T >> 4) - (int32_t)cal->T1)) >> 12) * (int32_t)cal->T3) >> 14;
    int32_t t_fine = v1 + v2;
    int32_t T = (t_fine * 5 + 128) >> 8;

    /* --- pressure: Bosch int64 -> P in Q24.8 Pa. `<<` on possibly-negative values
       is rewritten as multiplication by 2^n (same value, no UB). --- */
    int64_t p1 = (int64_t)t_fine - 128000;
    int64_t p2 = p1 * p1 * (int64_t)cal->P6;
    p2 += (p1 * (int64_t)cal->P5) * 131072;                 /* <<17 */
    p2 += (int64_t)cal->P4 * ((int64_t)1 << 35);            /* <<35 (constant, positive) */
    p1 = ((p1 * p1 * (int64_t)cal->P3) >> 8) + (p1 * (int64_t)cal->P2) * 4096; /* <<12 */
    p1 = (((int64_t)1 << 47) + p1) * (int64_t)cal->P1 >> 33;

    uint32_t press_pa;
    if (p1 == 0) {
        press_pa = 0;                                       /* avoid divide-by-zero */
    } else {
        int64_t p = 1048576 - adc_P;                        /* adc_P is 20-bit -> p > 0 */
        p = ((p << 31) - p2) * 3125 / p1;                   /* p<<31 safe (p>0); C `/` truncates */
        int64_t pv1 = ((int64_t)cal->P9 * (p >> 13) * (p >> 13)) >> 25;
        int64_t pv2 = ((int64_t)cal->P8 * p) >> 19;
        p = ((p + pv1 + pv2) >> 8) + (int64_t)cal->P7 * 16; /* <<4 */
        press_pa = (uint32_t)(p >> 8);                      /* Q24.8 -> Pa */
    }

    /* --- humidity: Bosch int32 -> H in Q22.10 %RH. H4<<20 rewritten as *2^20. --- */
    int32_t h = t_fine - 76800;
    h = ((((adc_H << 14) - ((int32_t)cal->H4 * 1048576) - ((int32_t)cal->H5 * h)) + 16384) >> 15) *
        (((((((h * (int32_t)cal->H6) >> 10) * (((h * (int32_t)cal->H3) >> 11) + 32768)) >> 10) + 2097152) * (int32_t)cal->H2 + 8192) >> 14);
    h = h - (((((h >> 15) * (h >> 15)) >> 7) * (int32_t)cal->H1) >> 4);
    h = (h < 0) ? 0 : h;
    h = (h > 419430400) ? 419430400 : h;
    uint32_t H = (uint32_t)(h >> 12);                       /* Q22.10 */

    out->temp_mc  = T * 10;
    out->press_pa = press_pa;
    out->humi_mrh = (uint32_t)(((uint64_t)H * 1000u) >> 10);
    return true;
}
```

- [ ] **Step 5: Run the test to verify it passes (GREEN)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ibme280 -o build/test_bme280_parse bme280/test_bme280_parse.c bme280/bme280_parse.c ../common/third_party/unity/unity.c && ./build/test_bme280_parse`
Expected: PASS — `4 Tests 0 Failures 0 Ignored` then `OK`.

- [ ] **Step 6: Strict + UBSan/ASan gates**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ibme280 -c bme280/bme280_parse.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK` (no warnings).
Run: `cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ibme280 -o build/san_bme280_parse bme280/test_bme280_parse.c bme280/bme280_parse.c ../common/third_party/unity/unity.c && ./build/san_bme280_parse`
Expected: PASS, no `runtime error:` lines (confirms no UB despite the heavy fixed-point math).

- [ ] **Step 7: Commit**

```bash
git add esp-libs/bme280/bme280_parse.h esp-libs/bme280/bme280_parse.c esp-libs/bme280/test_bme280_parse.c
git commit -m "feat(esp-libs/bme280): pure calib parse + Bosch compensation (host-tested)"
```

---

### Task 2: `bme280` — SDK glue over `i2c_bus` (compile-gate deferred)

**Files:**
- Create: `esp-libs/bme280/bme280.h`
- Create: `esp-libs/bme280/bme280.c`

**Interfaces:**
- Consumes: `bme280_parse.h` (`bme280_calib`, `bme280_reading`, `bme280_compensate`, `bme280_parse_calib`); `i2c_bus.h` (`i2c_bus_read_reg`, `i2c_bus_write_reg`).
- Produces: `bme280` struct, `bme280_init(bme280*, uint8_t addr)->esp_err_t`, `bme280_read(bme280*, bme280_reading*)->esp_err_t`; `BME280_ADDR_PRIMARY`/`BME280_ADDR_SECONDARY`.

> No host test — the glue needs the SDK (`i2c_bus`→`driver/i2c`, FreeRTOS). The `COMPILE_GATE` is **deferred** (xtensa toolchain absent). Write it to compile against the SDK; verification happens at Task 3's esp-gate link (also deferred) and on hardware later.

- [ ] **Step 1: Write the glue header** — `esp-libs/bme280/bme280.h`

```c
#ifndef CLIBS_BME280_H
#define CLIBS_BME280_H

#include <stdint.h>
#include "esp_err.h"
#include "bme280_parse.h"

#define BME280_ADDR_PRIMARY   0x76
#define BME280_ADDR_SECONDARY 0x77

/* BME280 on I2C. Caller runs i2c_bus_init(sda, scl) first. Not reentrant. */
typedef struct { uint8_t addr; bme280_calib calib; } bme280;

/* Probe chip id (reg 0xD0 must read 0x60), read + parse the two calib blocks,
 * and set ctrl_hum (osrs_h x1) BEFORE ctrl_meas (datasheet ordering quirk).
 * ESP_OK / ESP_ERR_NOT_FOUND (bad chip id) / ESP_ERR_INVALID_ARG (NULL self) /
 * propagated i2c_bus esp_err. */
esp_err_t bme280_init(bme280 *self, uint8_t addr);

/* Forced one-shot: write ctrl_meas (osrs_t x1 | osrs_p x1 | mode forced), wait
 * for the conversion, read the 8 raw bytes, and compensate into `out` (SI units).
 * ESP_OK / ESP_ERR_INVALID_ARG (NULL) / propagated i2c_bus esp_err. */
esp_err_t bme280_read(bme280 *self, bme280_reading *out);

#endif /* CLIBS_BME280_H */
```

- [ ] **Step 2: Write the glue implementation** — `esp-libs/bme280/bme280.c`

```c
#include "bme280.h"
#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BME280_REG_ID        0xD0
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_DATA      0xF7   /* 0xF7..0xFE: press[3] temp[3] hum[2] */
#define BME280_REG_CALIB_A   0x88   /* 0x88..0xA1 = 26 bytes */
#define BME280_REG_CALIB_H   0xE1   /* 0xE1..0xE7 = 7 bytes  */
#define BME280_CHIP_ID       0x60
#define BME280_OSRS_X1       0x01

esp_err_t bme280_init(bme280 *self, uint8_t addr)
{
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;

    uint8_t id = 0;
    esp_err_t err = i2c_bus_read_reg(addr, BME280_REG_ID, &id, 1);
    if (err != ESP_OK) {
        return err;
    }
    if (id != BME280_CHIP_ID) {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t calib26[26];
    uint8_t calibH[7];
    err = i2c_bus_read_reg(addr, BME280_REG_CALIB_A, calib26, sizeof calib26);
    if (err != ESP_OK) {
        return err;
    }
    err = i2c_bus_read_reg(addr, BME280_REG_CALIB_H, calibH, sizeof calibH);
    if (err != ESP_OK) {
        return err;
    }
    if (!bme280_parse_calib(calib26, calibH, &self->calib)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* ctrl_hum must be written before ctrl_meas to take effect (datasheet 5.4.3). */
    uint8_t ctrl_hum = BME280_OSRS_X1;
    return i2c_bus_write_reg(addr, BME280_REG_CTRL_HUM, &ctrl_hum, 1);
}

esp_err_t bme280_read(bme280 *self, bme280_reading *out)
{
    if (!self || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    /* forced mode: osrs_t x1 (bits 7:5) | osrs_p x1 (bits 4:2) | mode = forced (0b01). */
    uint8_t ctrl_meas = (uint8_t)((BME280_OSRS_X1 << 5) | (BME280_OSRS_X1 << 2) | 0x01);
    esp_err_t err = i2c_bus_write_reg(self->addr, BME280_REG_CTRL_MEAS, &ctrl_meas, 1);
    if (err != ESP_OK) {
        return err;
    }

    /* wait for the measurement (status bit 3 = measuring), bounded to ~50 ms. */
    for (int i = 0; i < 10; i++) {
        uint8_t status = 0;
        err = i2c_bus_read_reg(self->addr, BME280_REG_STATUS, &status, 1);
        if (err != ESP_OK) {
            return err;
        }
        if ((status & 0x08) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t raw[8];
    err = i2c_bus_read_reg(self->addr, BME280_REG_DATA, raw, sizeof raw);
    if (err != ESP_OK) {
        return err;
    }

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  |  raw[7];

    if (!bme280_compensate(&self->calib, adc_T, adc_P, adc_H, out)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}
```

- [ ] **Step 3: Confirm the pure part still builds (the glue's compile-gate is deferred)**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ibme280 -c bme280/bme280_parse.c -o /dev/null && echo OK`
Expected: `OK`. (Do NOT try to host-compile `bme280.c` — it needs SDK headers. It will be link-checked by the deferred esp-gate build.)

- [ ] **Step 4: Commit**

```bash
git add esp-libs/bme280/bme280.h esp-libs/bme280/bme280.c
git commit -m "feat(esp-libs/bme280): I2C glue over i2c_bus, forced-mode read (compile-gate deferred)"
```

---

### Task 3: Integration — wire into Makefile + component.mk + esp-gate + README

**Files:**
- Modify: `esp-libs/Makefile`
- Modify: `esp-libs/component.mk`
- Modify: `esp-gate/main/main.c`
- Modify: `esp-libs/README.md`

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces: `make -C esp-libs test` → 19 suites; component build includes bme280; esp-gate touches a bme280 symbol.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add `bme280_parse` to `.PHONY` + the `test:` aggregate, a run target, and a `strict:` line.

Add `bme280_parse` to the `.PHONY` list and to the `test:` aggregate line (after `saturating_counter`).

Add this target (after the existing `hcsr04_dist`/`saturating_counter` targets, before `strict:`):
```makefile
bme280_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibme280 -o $(BUILD)/test_bme280_parse \
	    bme280/test_bme280_parse.c bme280/bme280_parse.c $(UNITY)/unity.c
	./$(BUILD)/test_bme280_parse
```

Add this line to the `strict:` recipe (before its `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibme280 -c bme280/bme280_parse.c -o /dev/null
```

- [ ] **Step 2: Run the full host gate**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `19` (18 existing + bme280_parse).
Run: `make -C esp-libs strict`
Expected: `strict: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add `bme280` to the include-dirs + srcdirs lists and its objects to `COMPONENT_OBJS`.

Add `bme280` to the `COMPONENT_ADD_INCLUDEDIRS` list and the `COMPONENT_SRCDIRS` list (alongside the other lib dirs). Add to `COMPONENT_OBJS`:
```makefile
                  bme280/bme280.o bme280/bme280_parse.o \
```
(Place it with the other `.o` entries; the `test_*.c` files are never listed in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include the header and touch a symbol.

Add near the other includes:
```c
#include "bme280.h"
```
Add inside the gate's touch body (alongside the other `(void)`-symbol touches), referencing a pure symbol so it links without hardware:
```c
    bme280_calib bme_cal = {0};
    bme280_reading bme_r;
    (void) bme280_parse_calib((const uint8_t[26]){0}, (const uint8_t[7]){0}, &bme_cal);
    (void) bme280_compensate(&bme_cal, 0, 0, 0, &bme_r);
    bme280 bme_dev;
    (void) bme280_init(&bme_dev, BME280_ADDR_PRIMARY);
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add an L3 row.

Add to the Layer-3 driver list/table:
```markdown
### `bme280` — T/H/P over I2C
- `bme280_parse_calib` + `bme280_compensate` (pure, host-tested): Bosch fixed-point
  compensation → SI units (milli-°C / milli-%RH / Pa).
- `bme280_init(&d, BME280_ADDR_PRIMARY)` then `bme280_read(&d, &r)` — forced-mode
  one-shot over `i2c_bus`. Runtime verified on hardware later.
```

- [ ] **Step 6: Re-run host gate + attempt the (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict`
Expected: `19` and `strict: OK`.
ESP compile-gate (only if the xtensa toolchain is present — otherwise record DEFERRED):
```bash
# export IDF_PATH=~/esp/ESP8266_RTOS_SDK ; export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
# cd esp-gate && make defconfig && make    # -> build/esp_libs_gate.elf  (DEFERRED if toolchain absent)
```

```bash
git add esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
git commit -m "chore(esp-libs): wire bme280 into Makefile/component.mk/esp-gate/README (test->19)"
```

---

## Self-Review

**1. Spec coverage:** `bme280_parse_calib` + `bme280_compensate` with the exact cross-checked vectors and the H4/H5 sign-extension case → Task 1. SI units → Task 1 impl. Glue `bme280_init`/`bme280_read` forced-mode over i2c_bus, ctrl_hum-before-ctrl_meas, raw extraction, chip-id check → Task 2. Build infra (Makefile→19, component.mk, esp-gate, README) → Task 3. Deferred xtensa gate + hardware → noted in Tasks 2/3. All spec sections covered.

**2. Placeholder scan:** none — full code in every code step; exact commands + expected output in every run step; no "handle edge cases"/"similar to". Clean.

**3. Type consistency:** `bme280_calib`/`bme280_reading`/`bme280` and the function signatures are identical across header, impl, test, glue, and esp-gate touch. `temp_mc`(int32)/`humi_mrh`(uint32)/`press_pa`(uint32) consistent. Vectors (20440/101576/42743) match the spec. `bme280_parse_calib(const uint8_t[26], const uint8_t[7], bme280_calib*)` consistent everywhere. UB-safe rewrites (`*131072`, `*((int64_t)1<<35)`, `*4096`, `*16`, `*1048576`) preserve the spec's documented values.
