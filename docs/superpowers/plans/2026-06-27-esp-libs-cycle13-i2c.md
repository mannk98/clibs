# esp-libs cycle 13 — I2C sensors + RTC (bh1750, sht3x, ds3231) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three independent L3 I2C devices to esp-libs — `bh1750` (light), `sht3x` (T/H), `ds3231` (RTC) — each a host-tested pure part + SDK glue, plus an `i2c_bus` raw read/write pre-step the command-based sensors need.

**Architecture:** Task 0 extends `i2c_bus` with raw (no-register) read/write as a direct glue-only change (no RED test possible for L2 glue). Tasks 1–3 are header-split L3 drivers: pure `<lib>.{h,c}` (host-Unity-tested) + SDK glue `<lib>_dev.{h,c}` (xtensa COMPILE_GATE deferred). Task 4 wires the build. `bh1750`/`sht3x` consume the Task-0 raw API; `ds3231` uses the existing register API.

**Tech Stack:** C11, host `cc` + Unity (`../common/third_party/unity`), GNU make; ESP8266 RTOS SDK for the glue (deferred).

**Spec:** `docs/superpowers/specs/2026-06-27-esp-libs-cycle13-i2c-sensors-rtc-design.md`

## Global Constraints

- **Branch `feat/esp-libs-cycle13-i2c`** (`git checkout -b feat/esp-libs-cycle13-i2c` before Task 0).
- **Commands run from `clibs/esp-libs/`.** Unity at `../common/third_party/unity`.
- **Pure files** freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable.
- **Glue files** (`*_dev.c`, `i2c_bus.c`) SDK (`esp_err.h`, `driver/i2c.h`, `rom/ets_sys.h`) → **not host-runnable; COMPILE_GATE deferred** (xtensa absent).
- **No UB:** shifts on `uint8`/`uint16`/`uint32` positive operands only; SHT3x conversion widens to `int64` before the multiply; no left-shift of a possibly-negative value.
- **Guards** `CLIBS_<MODULE>_H`; doc comment per public fn/struct; NULL-guard every pointer param; "Not reentrant".
- **Units:** milli-units (`temp_mc` milli-°C, `humi_mrh` milli-%RH, `milli_lux`).
- **Flags:** host `-std=c11 -Wall -Wextra -g`; strict `-std=c11 -Wall -Wextra -Werror`; sanitize `-std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all` (`ASAN_OPTIONS=detect_leaks=0` on darwin).

---

### Task 0: `i2c_bus` raw read/write (direct, glue-only — no RED test)

**Files:** Modify `esp-libs/i2c_bus/i2c_bus.h`, `esp-libs/i2c_bus/i2c_bus.c`, `esp-gate/main/main.c`

**Interfaces:**
- Produces: `esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)` (raw, no register; data NULL iff len==0), `esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len)` (raw, no register; data!=NULL, len>0).

> L2 SDK glue has no host runtime → no RED test. Verify by host-gate regression (unchanged) + the deferred COMPILE_GATE's reviewer substitute.

- [ ] **Step 1: Add the raw declarations** — append to `esp-libs/i2c_bus/i2c_bus.h` before `#endif`

```c
/* Raw write: START, addr+W, data[0..len-1], STOP. No register/pointer byte.
 * data may be NULL iff len == 0. */
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);

/* Raw read: START, addr+R, read len bytes (last NACK), STOP. No register write first.
 * data != NULL, len > 0. */
esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len);
```

- [ ] **Step 2: Add the raw definitions** — append to `esp-libs/i2c_bus/i2c_bus.c`

```c
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_WRITE), true);
    if (len > 0) {
        i2c_master_write(cmd, (uint8_t *) data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_READ), true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}
```

- [ ] **Step 3: esp-gate symbol touch** — in `esp-gate/main/main.c`, in the existing i2c block (right after the `i2c_bus_read_reg(...)` line), add:

```c
    uint8_t i2craw[2] = {0};
    (void) i2c_bus_write(0x23, (const uint8_t[]){0x10}, 1);
    (void) i2c_bus_read(0x23, i2craw, 2);
```

- [ ] **Step 4: Host-gate regression (must be unchanged)**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `29` (no new pure suite — glue only).
Run: `make -C esp-libs strict`
Expected: `strict: OK`.

- [ ] **Step 5: Commit** (review substitutes the deferred COMPILE_GATE)

```bash
git add esp-libs/i2c_bus/ esp-gate/main/main.c
git commit -m "feat(esp-libs/i2c_bus): raw read/write (no register) for command-based I2C devices"
```

---

### Task 1: `bh1750` — ambient light (pure + glue)

**Files:** Create `esp-libs/bh1750/bh1750.h`, `bh1750.c`, `bh1750_dev.h`, `bh1750_dev.c`; Test `esp-libs/bh1750/test_bh1750.c`

**Interfaces:**
- Consumes: `i2c_bus_write`, `i2c_bus_read` (Task 0).
- Produces: `bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux)`; glue `bh1750 {uint8_t addr}`, `bh1750_init`, `bh1750_read`.

- [ ] **Step 1: Write the failing test** — `esp-libs/bh1750/test_bh1750.c`

```c
#include "unity.h"
#include "bh1750.h"

void setUp(void) {}
void tearDown(void) {}

static void test_parse_5lux(void) {
    uint8_t raw[2] = {0x00, 0x06};   /* val 6 -> 5 lux */
    uint32_t mlx = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &mlx));
    TEST_ASSERT_EQUAL_UINT32(5000u, mlx);
}

static void test_parse_10lux(void) {
    uint8_t raw[2] = {0x00, 0x0C};   /* val 12 -> 10 lux */
    uint32_t mlx = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &mlx));
    TEST_ASSERT_EQUAL_UINT32(10000u, mlx);
}

static void test_parse_max(void) {
    uint8_t raw[2] = {0xFF, 0xFF};   /* val 65535 */
    uint32_t mlx = 0;
    TEST_ASSERT_TRUE(bh1750_parse(raw, &mlx));
    TEST_ASSERT_EQUAL_UINT32(54612500u, mlx);
}

static void test_null(void) {
    uint8_t raw[2] = {0x00, 0x06};
    uint32_t mlx = 0;
    TEST_ASSERT_FALSE(bh1750_parse(NULL, &mlx));
    TEST_ASSERT_FALSE(bh1750_parse(raw, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_5lux);
    RUN_TEST(test_parse_10lux);
    RUN_TEST(test_parse_max);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/bh1750/bh1750.h`

```c
#ifndef CLIBS_BH1750_H
#define CLIBS_BH1750_H

#include <stdint.h>
#include <stdbool.h>

/* BH1750 16-bit big-endian raw count (raw[0]=MSB) -> milli-lux. lux = raw / 1.2,
 * so milli_lux = raw * 2500 / 3 (exact in uint32: 65535*2500 < 2^32). NULL -> false. */
bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux);

#endif /* CLIBS_BH1750_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ibh1750 -o build/test_bh1750 bh1750/test_bh1750.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_bh1750_parse`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/bh1750/bh1750.c`

```c
#include "bh1750.h"

bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux)
{
    if (raw == NULL || milli_lux == NULL) {
        return false;
    }
    uint32_t val = ((uint32_t) raw[0] << 8) | (uint32_t) raw[1];
    *milli_lux = val * 2500u / 3u;
    return true;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ibh1750 -o build/test_bh1750 bh1750/test_bh1750.c bh1750/bh1750.c ../common/third_party/unity/unity.c && ./build/test_bh1750`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Ibh1750 -c bh1750/bh1750.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ibh1750 -o build/san_bh1750 bh1750/test_bh1750.c bh1750/bh1750.c ../common/third_party/unity/unity.c && ./build/san_bh1750'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/bh1750/bh1750_dev.h` then `bh1750_dev.c`

`bh1750_dev.h`:
```c
#ifndef CLIBS_BH1750_DEV_H
#define CLIBS_BH1750_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "bh1750.h"

/* BH1750 ambient-light sensor on i2c_bus. addr 0x23 (ADDR low) or 0x5C (high). Not reentrant. */
typedef struct { uint8_t addr; } bh1750;

esp_err_t bh1750_init(bh1750 *self, uint8_t addr);         /* set continuous high-res mode */
esp_err_t bh1750_read(bh1750 *self, uint32_t *milli_lux);  /* read 2 bytes -> bh1750_parse */

#endif /* CLIBS_BH1750_DEV_H */
```

`bh1750_dev.c` (not host-compiled):
```c
#include "bh1750_dev.h"
#include "i2c_bus.h"

#define BH1750_CMD_CONT_HRES 0x10   /* continuously high-resolution mode (1 lx) */

esp_err_t bh1750_init(bh1750 *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    return i2c_bus_write(addr, &cmd, 1);
}

esp_err_t bh1750_read(bh1750 *self, uint32_t *milli_lux)
{
    if (self == NULL || milli_lux == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t raw[2] = {0};
    esp_err_t err = i2c_bus_read(self->addr, raw, 2);   /* needs ~120 ms after mode set (first cut) */
    if (err != ESP_OK) {
        return err;
    }
    return bh1750_parse(raw, milli_lux) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ibh1750 -c bh1750/bh1750.c -o /dev/null && echo OK`  (do NOT host-compile bh1750_dev.c)
```bash
git add esp-libs/bh1750/
git commit -m "feat(esp-libs/bh1750): ambient-light raw->milli-lux (pure, host-tested) + i2c glue (deferred)"
```

---

### Task 2: `sht3x` — temperature + humidity (pure + glue)

**Files:** Create `esp-libs/sht3x/sht3x.h`, `sht3x.c`, `sht3x_dev.h`, `sht3x_dev.c`; Test `esp-libs/sht3x/test_sht3x.c`

**Interfaces:**
- Consumes: `i2c_bus_write`, `i2c_bus_read` (Task 0).
- Produces: `bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh)`; glue `sht3x {uint8_t addr}`, `sht3x_init`, `sht3x_read`.

- [ ] **Step 1: Write the failing test** — `esp-libs/sht3x/test_sht3x.c`

```c
#include "unity.h"
#include "sht3x.h"

void setUp(void) {}
void tearDown(void) {}

/* Sensirion CRC8 (poly 0x31, init 0xFF): CRC8({0x00,0x00})=0x81, CRC8({0xBE,0xEF})=0x92. */

static void test_parse_zero(void) {
    uint8_t raw[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x81};
    int32_t t = 0, h = 0;
    TEST_ASSERT_TRUE(sht3x_parse(raw, &t, &h));
    TEST_ASSERT_EQUAL_INT32(-45000, t);   /* -45 + 175*0/65535 */
    TEST_ASSERT_EQUAL_INT32(0, h);
}

static void test_parse_beef(void) {
    uint8_t raw[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x92};   /* raw 0xBEEF = 48879 */
    int32_t t = 0, h = 0;
    TEST_ASSERT_TRUE(sht3x_parse(raw, &t, &h));
    TEST_ASSERT_EQUAL_INT32(85523, t);    /* -45000 + 175000*48879/65535 */
    TEST_ASSERT_EQUAL_INT32(74584, h);    /* 100000*48879/65535 */
}

static void test_bad_crc(void) {
    uint8_t raw[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x81};   /* first CRC wrong */
    int32_t t = 0, h = 0;
    TEST_ASSERT_FALSE(sht3x_parse(raw, &t, &h));
}

static void test_null(void) {
    uint8_t raw[6] = {0x00, 0x00, 0x81, 0x00, 0x00, 0x81};
    int32_t t = 0, h = 0;
    TEST_ASSERT_FALSE(sht3x_parse(NULL, &t, &h));
    TEST_ASSERT_FALSE(sht3x_parse(raw, NULL, &h));
    TEST_ASSERT_FALSE(sht3x_parse(raw, &t, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_zero);
    RUN_TEST(test_parse_beef);
    RUN_TEST(test_bad_crc);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/sht3x/sht3x.h`

```c
#ifndef CLIBS_SHT3X_H
#define CLIBS_SHT3X_H

#include <stdint.h>
#include <stdbool.h>

/* SHT3x 6-byte frame [t_msb,t_lsb,t_crc, h_msb,h_lsb,h_crc]. Validate both Sensirion
 * CRC8 (poly 0x31, init 0xFF, MSB-first, no final xor) over each 2-byte group; any
 * mismatch -> false. Then (int64 intermediate, no overflow / no signed shift):
 *   temp_mc  = -45000 + 175000 * raw_t / 65535   (milli-degC)
 *   humi_mrh =          100000 * raw_h / 65535   (milli-%RH)
 * raw_t = (t_msb<<8)|t_lsb, raw_h = (h_msb<<8)|h_lsb. NULL -> false. */
bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh);

#endif /* CLIBS_SHT3X_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Isht3x -o build/test_sht3x sht3x/test_sht3x.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_sht3x_parse`.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/sht3x/sht3x.c`

```c
#include "sht3x.h"
#include <stddef.h>

/* Sensirion CRC8: poly 0x31, init 0xFF, MSB-first, no final xor. Operates on uint8
 * (promoted to non-negative int) — no left-shift of a negative value. */
static uint8_t sht3x_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80u) ? (uint8_t) ((crc << 1) ^ 0x31u) : (uint8_t) (crc << 1);
        }
    }
    return crc;
}

bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh)
{
    if (raw == NULL || temp_mc == NULL || humi_mrh == NULL) {
        return false;
    }
    if (sht3x_crc8(&raw[0], 2) != raw[2] || sht3x_crc8(&raw[3], 2) != raw[5]) {
        return false;
    }
    uint32_t raw_t = ((uint32_t) raw[0] << 8) | (uint32_t) raw[1];
    uint32_t raw_h = ((uint32_t) raw[3] << 8) | (uint32_t) raw[4];
    /* widen to int64 BEFORE the multiply (map_range lesson); results fit int32. */
    *temp_mc  = (int32_t) (-45000 + (int64_t) 175000 * raw_t / 65535);
    *humi_mrh = (int32_t) ((int64_t) 100000 * raw_h / 65535);
    return true;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Isht3x -o build/test_sht3x sht3x/test_sht3x.c sht3x/sht3x.c ../common/third_party/unity/unity.c && ./build/test_sht3x`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Isht3x -c sht3x/sht3x.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Isht3x -o build/san_sht3x sht3x/test_sht3x.c sht3x/sht3x.c ../common/third_party/unity/unity.c && ./build/san_sht3x'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/sht3x/sht3x_dev.h` then `sht3x_dev.c`

`sht3x_dev.h`:
```c
#ifndef CLIBS_SHT3X_DEV_H
#define CLIBS_SHT3X_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "sht3x.h"

/* SHT3x temperature/humidity sensor on i2c_bus. addr 0x44 (default) or 0x45. Not reentrant. */
typedef struct { uint8_t addr; } sht3x;

esp_err_t sht3x_init(sht3x *self, uint8_t addr);
esp_err_t sht3x_read(sht3x *self, int32_t *temp_mc, int32_t *humi_mrh);

#endif /* CLIBS_SHT3X_DEV_H */
```

`sht3x_dev.c` (not host-compiled):
```c
#include "sht3x_dev.h"
#include "i2c_bus.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

esp_err_t sht3x_init(sht3x *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    return ESP_OK;
}

esp_err_t sht3x_read(sht3x *self, int32_t *temp_mc, int32_t *humi_mrh)
{
    if (self == NULL || temp_mc == NULL || humi_mrh == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t cmd[2] = {0x24, 0x00};   /* single-shot, high repeatability, clock-stretch off */
    esp_err_t err = i2c_bus_write(self->addr, cmd, 2);
    if (err != ESP_OK) {
        return err;
    }
    ets_delay_us(15000);             /* high-rep measurement ~15 ms (first cut) */
    uint8_t raw[6] = {0};
    err = i2c_bus_read(self->addr, raw, 6);
    if (err != ESP_OK) {
        return err;
    }
    return sht3x_parse(raw, temp_mc, humi_mrh) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Isht3x -c sht3x/sht3x.c -o /dev/null && echo OK`  (do NOT host-compile sht3x_dev.c)
```bash
git add esp-libs/sht3x/
git commit -m "feat(esp-libs/sht3x): T/H parse with Sensirion CRC8 (pure, host-tested) + i2c glue (deferred)"
```

---

### Task 3: `ds3231` — real-time clock (pure + glue)

**Files:** Create `esp-libs/ds3231/ds3231.h`, `ds3231.c`, `ds3231_dev.h`, `ds3231_dev.c`; Test `esp-libs/ds3231/test_ds3231.c`

**Interfaces:**
- Consumes: `i2c_bus_read_reg`, `i2c_bus_write_reg` (existing — no Task-0 dependency).
- Produces: `ds3231_time {uint8_t sec,min,hour,dow,date,month; uint16_t year}`; `bool ds3231_decode(const uint8_t raw[7], ds3231_time*)`, `bool ds3231_encode(const ds3231_time*, uint8_t raw[7])`, `int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb)`; glue `ds3231 {uint8_t addr}`, `ds3231_init`, `ds3231_get`, `ds3231_set`, `ds3231_read_temp`.

- [ ] **Step 1: Write the failing test** — `esp-libs/ds3231/test_ds3231.c`

```c
#include "unity.h"
#include "ds3231.h"

void setUp(void) {}
void tearDown(void) {}

/* BCD of 2026-06-27 (Sat=dow6) 13:45:30 */
static void test_decode(void) {
    uint8_t raw[7] = {0x30, 0x45, 0x13, 0x06, 0x27, 0x06, 0x26};
    ds3231_time t;
    TEST_ASSERT_TRUE(ds3231_decode(raw, &t));
    TEST_ASSERT_EQUAL_UINT8(30, t.sec);
    TEST_ASSERT_EQUAL_UINT8(45, t.min);
    TEST_ASSERT_EQUAL_UINT8(13, t.hour);
    TEST_ASSERT_EQUAL_UINT8(6, t.dow);
    TEST_ASSERT_EQUAL_UINT8(27, t.date);
    TEST_ASSERT_EQUAL_UINT8(6, t.month);
    TEST_ASSERT_EQUAL_UINT16(2026, t.year);
}

static void test_encode_roundtrip(void) {
    uint8_t raw[7] = {0x30, 0x45, 0x13, 0x06, 0x27, 0x06, 0x26};
    ds3231_time t;
    TEST_ASSERT_TRUE(ds3231_decode(raw, &t));
    uint8_t out[7] = {0};
    TEST_ASSERT_TRUE(ds3231_encode(&t, out));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(raw, out, 7);
}

static void test_temp(void) {
    TEST_ASSERT_EQUAL_INT32(25250, ds3231_temp_mc(0x19, 0x40));   /* 25 + 1*0.25 */
    TEST_ASSERT_EQUAL_INT32(-500,  ds3231_temp_mc(0xFF, 0x80));   /* -1 + 2*0.25 = -0.5 */
}

static void test_null(void) {
    uint8_t raw[7] = {0};
    ds3231_time t = {0};
    TEST_ASSERT_FALSE(ds3231_decode(NULL, &t));
    TEST_ASSERT_FALSE(ds3231_decode(raw, NULL));
    TEST_ASSERT_FALSE(ds3231_encode(NULL, raw));
    TEST_ASSERT_FALSE(ds3231_encode(&t, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_decode);
    RUN_TEST(test_encode_roundtrip);
    RUN_TEST(test_temp);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write the pure header** — `esp-libs/ds3231/ds3231.h`

```c
#ifndef CLIBS_DS3231_H
#define CLIBS_DS3231_H

#include <stdint.h>
#include <stdbool.h>

typedef struct { uint8_t sec, min, hour, dow, date, month; uint16_t year; } ds3231_time;

/* Decode the 7 timekeeping registers 0x00..0x06 (all BCD, 24-hour mode). NULL -> false. */
bool ds3231_decode(const uint8_t raw[7], ds3231_time *out);

/* Encode a ds3231_time into 7 BCD registers (year % 100). NULL -> false. */
bool ds3231_encode(const ds3231_time *t, uint8_t raw[7]);

/* On-chip temperature regs 0x11 (signed integer degC) + 0x12 (bits 7..6 = quarters):
 * temp_mc = (int8_t)msb * 1000 + (lsb >> 6) * 250. */
int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb);

#endif /* CLIBS_DS3231_H */
```

- [ ] **Step 3: Run the test to verify it fails (RED)**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ids3231 -o build/test_ds3231 ds3231/test_ds3231.c ../common/third_party/unity/unity.c`
Expected: FAIL — undefined reference to `_ds3231_decode` etc.

- [ ] **Step 4: Write the pure implementation** — `esp-libs/ds3231/ds3231.c`

```c
#include "ds3231.h"

static uint8_t bcd2bin(uint8_t v) { return (uint8_t) ((v >> 4) * 10u + (v & 0x0Fu)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t) (((v / 10u) << 4) | (v % 10u)); }

bool ds3231_decode(const uint8_t raw[7], ds3231_time *out)
{
    if (raw == NULL || out == NULL) {
        return false;
    }
    out->sec   = bcd2bin(raw[0] & 0x7Fu);
    out->min   = bcd2bin(raw[1] & 0x7Fu);
    out->hour  = bcd2bin(raw[2] & 0x3Fu);   /* 24-hour mode */
    out->dow   = raw[3] & 0x07u;
    out->date  = bcd2bin(raw[4] & 0x3Fu);
    out->month = bcd2bin(raw[5] & 0x1Fu);
    out->year  = (uint16_t) (2000u + bcd2bin(raw[6]));
    return true;
}

bool ds3231_encode(const ds3231_time *t, uint8_t raw[7])
{
    if (t == NULL || raw == NULL) {
        return false;
    }
    raw[0] = bin2bcd(t->sec);
    raw[1] = bin2bcd(t->min);
    raw[2] = bin2bcd(t->hour);
    raw[3] = (uint8_t) (t->dow & 0x07u);
    raw[4] = bin2bcd(t->date);
    raw[5] = bin2bcd(t->month);
    raw[6] = bin2bcd((uint8_t) (t->year % 100u));
    return true;
}

int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb)
{
    return (int32_t) (int8_t) msb * 1000 + (int32_t) (lsb >> 6) * 250;
}
```

- [ ] **Step 5: GREEN + strict + sanitize**

Run: `mkdir -p build && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ids3231 -o build/test_ds3231 ds3231/test_ds3231.c ds3231/ds3231.c ../common/third_party/unity/unity.c && ./build/test_ds3231`
Expected: PASS — `4 Tests 0 Failures 0 Ignored`, `OK`.
Run: `cc -std=c11 -Wall -Wextra -Werror -Ids3231 -c ds3231/ds3231.c -o /dev/null && echo STRICT_OK`
Expected: `STRICT_OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 sh -c 'cc -std=c11 -fsanitize=undefined,address -fno-sanitize-recover=all -I../common/third_party/unity -Ids3231 -o build/san_ds3231 ds3231/test_ds3231.c ds3231/ds3231.c ../common/third_party/unity/unity.c && ./build/san_ds3231'`
Expected: PASS, no `runtime error:` lines.

- [ ] **Step 6: Write the SDK glue (deferred)** — `esp-libs/ds3231/ds3231_dev.h` then `ds3231_dev.c`

`ds3231_dev.h`:
```c
#ifndef CLIBS_DS3231_DEV_H
#define CLIBS_DS3231_DEV_H

#include <stdint.h>
#include "esp_err.h"
#include "ds3231.h"

/* DS3231 RTC on i2c_bus (register-mapped; addr 0x68). Not reentrant. */
typedef struct { uint8_t addr; } ds3231;

esp_err_t ds3231_init(ds3231 *self, uint8_t addr);            /* 0x68 */
esp_err_t ds3231_get(ds3231 *self, ds3231_time *out);         /* read regs 0x00..0x06 -> decode */
esp_err_t ds3231_set(ds3231 *self, const ds3231_time *t);     /* encode -> write regs 0x00..0x06 */
esp_err_t ds3231_read_temp(ds3231 *self, int32_t *temp_mc);   /* read regs 0x11/0x12 -> temp_mc */

#endif /* CLIBS_DS3231_DEV_H */
```

`ds3231_dev.c` (not host-compiled):
```c
#include "ds3231_dev.h"
#include "i2c_bus.h"

#define DS3231_REG_TIME 0x00
#define DS3231_REG_TEMP 0x11

esp_err_t ds3231_init(ds3231 *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    return ESP_OK;
}

esp_err_t ds3231_get(ds3231 *self, ds3231_time *out)
{
    if (self == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t raw[7] = {0};
    esp_err_t err = i2c_bus_read_reg(self->addr, DS3231_REG_TIME, raw, 7);
    if (err != ESP_OK) {
        return err;
    }
    return ds3231_decode(raw, out) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t ds3231_set(ds3231 *self, const ds3231_time *t)
{
    if (self == NULL || t == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t raw[7] = {0};
    if (!ds3231_encode(t, raw)) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_bus_write_reg(self->addr, DS3231_REG_TIME, raw, 7);
}

esp_err_t ds3231_read_temp(ds3231 *self, int32_t *temp_mc)
{
    if (self == NULL || temp_mc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t raw[2] = {0};
    esp_err_t err = i2c_bus_read_reg(self->addr, DS3231_REG_TEMP, raw, 2);
    if (err != ESP_OK) {
        return err;
    }
    *temp_mc = ds3231_temp_mc(raw[0], raw[1]);
    return ESP_OK;
}
```

- [ ] **Step 7: Confirm pure builds, then commit**

Run: `cc -std=c11 -Wall -Wextra -Werror -Ids3231 -c ds3231/ds3231.c -o /dev/null && echo OK`  (do NOT host-compile ds3231_dev.c)
```bash
git add esp-libs/ds3231/
git commit -m "feat(esp-libs/ds3231): RTC BCD decode/encode + on-chip temp (pure, host-tested) + i2c glue (deferred)"
```

---

### Task 4: Integration — Makefile + component.mk + esp-gate + README

**Files:** Modify `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`

**Interfaces:** Consumes Tasks 1–3. Produces `make -C esp-libs test` → 32 suites.

- [ ] **Step 1: Wire `esp-libs/Makefile`** — add 3 suites to `.PHONY` + `test:`, a run target each, 3 strict lines.

Add `bh1750 sht3x ds3231` to the `.PHONY` line and to the `test:` aggregate line.

Add these three targets (after the existing `hx711` target, before `strict:`):
```makefile
bh1750: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibh1750 -o $(BUILD)/test_bh1750 \
	    bh1750/test_bh1750.c bh1750/bh1750.c $(UNITY)/unity.c
	./$(BUILD)/test_bh1750

sht3x: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Isht3x -o $(BUILD)/test_sht3x \
	    sht3x/test_sht3x.c sht3x/sht3x.c $(UNITY)/unity.c
	./$(BUILD)/test_sht3x

ds3231: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ids3231 -o $(BUILD)/test_ds3231 \
	    ds3231/test_ds3231.c ds3231/ds3231.c $(UNITY)/unity.c
	./$(BUILD)/test_ds3231
```

Add these three lines to the `strict:` recipe (before its `@echo "strict: OK"`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibh1750 -c bh1750/bh1750.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Isht3x  -c sht3x/sht3x.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ids3231 -c ds3231/ds3231.c -o /dev/null
```

- [ ] **Step 2: Run the host gate set**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$"`
Expected: `32`.
Run: `make -C esp-libs strict`
Expected: `strict: OK`.
Run: `ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `sanitize: OK`.

- [ ] **Step 3: Wire `esp-libs/component.mk`** — add dirs + objects.

Add `bh1750 sht3x ds3231` to `COMPONENT_ADD_INCLUDEDIRS` and `COMPONENT_SRCDIRS`. Add to `COMPONENT_OBJS` (with the other `.o` entries):
```makefile
                  bh1750/bh1750.o bh1750/bh1750_dev.o \
                  sht3x/sht3x.o sht3x/sht3x_dev.o \
                  ds3231/ds3231.o ds3231/ds3231_dev.o \
```
(`test_*.c` never in `COMPONENT_OBJS`.)

- [ ] **Step 4: Wire `esp-gate/main/main.c`** — include glue headers + touch one symbol per lib.

Add near the other includes:
```c
#include "bh1750_dev.h"
#include "sht3x_dev.h"
#include "ds3231_dev.h"
```
Add inside the gate's touch body (after the i2c raw touch from Task 0):
```c
    bh1750 lux;
    uint32_t mlx = 0;
    (void) bh1750_init(&lux, 0x23);
    (void) bh1750_read(&lux, &mlx);

    sht3x th;
    int32_t sht_t = 0, sht_h = 0;
    (void) sht3x_init(&th, 0x44);
    (void) sht3x_read(&th, &sht_t, &sht_h);

    ds3231 rtc;
    ds3231_time now;
    (void) ds3231_init(&rtc, 0x68);
    (void) ds3231_get(&rtc, &now);
    int32_t rtc_t = 0;
    (void) ds3231_read_temp(&rtc, &rtc_t);
    ESP_LOGI(TAG, "i2c-dev mlx=%u sht_t=%d ds_t=%d", (unsigned) mlx, (int) sht_t, (int) rtc_t);
```

- [ ] **Step 5: Update `esp-libs/README.md`** — add rows.

Add to the Layer-3 driver section:
```markdown
### `bh1750` — ambient light (I2C)
- `bh1750_parse` (pure, host-tested): 16-bit raw → milli-lux (raw/1.2). `bh1750_init` /
  `bh1750_read` over `i2c_bus` (continuous high-res mode).

### `sht3x` — temperature + humidity (I2C)
- `sht3x_parse` (pure, host-tested): validate 2× Sensirion CRC8 → milli-°C / milli-%RH.
  `sht3x_init` / `sht3x_read` over `i2c_bus` (single-shot high repeatability).

### `ds3231` — real-time clock (I2C)
- `ds3231_decode` / `ds3231_encode` / `ds3231_temp_mc` (pure, host-tested): BCD↔time +
  on-chip temperature. `ds3231_init` / `ds3231_get` / `ds3231_set` / `ds3231_read_temp`.
```

Also extend the `i2c_bus` README entry to note the raw API:
```markdown
- `i2c_bus_write(addr, data, len)` / `i2c_bus_read(addr, data, len)` — raw transfers with
  no register/pointer byte, for command-based devices (BH1750, SHT3x).
```

- [ ] **Step 6: Re-run host gates + (deferred) esp-gate link, then commit**

Run: `make -C esp-libs test 2>&1 | grep -c "^OK$" && make -C esp-libs strict && ASAN_OPTIONS=detect_leaks=0 make -C esp-libs sanitize 2>&1 | tail -1`
Expected: `32`, `strict: OK`, `sanitize: OK`.
ESP compile-gate (only if xtensa present; else record DEFERRED):
```bash
# cd esp-gate && make defconfig && make   # -> build/esp_libs_gate.elf (DEFERRED if toolchain absent)
```
```bash
git add esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c esp-libs/README.md
git commit -m "chore(esp-libs): wire bh1750/sht3x/ds3231 into Makefile/component.mk/esp-gate/README (test->32)"
```

---

## Self-Review

**1. Spec coverage:** i2c_bus raw read/write → Task 0. bh1750 raw→milli-lux + vectors → Task 1. sht3x Sensirion-CRC8 + int64 conversion + vectors (`{0x00,0x00}→0x81`, `{0xBE,0xEF}→0x92`) → Task 2. ds3231 BCD decode/encode + temp + vectors → Task 3. Makefile→32, component.mk, esp-gate, README → Task 4. All spec sections covered. DS3231 correctly uses the existing register API (no Task-0 dep); bh1750/sht3x consume Task 0.

**2. Placeholder scan:** none — full code in every code step (CRC8, int64 conversion, BCD helpers, all vectors), exact commands + expected output; glue is real first-cut code (deferred), not stubs.

**3. Type consistency:** pure names (`bh1750`,`sht3x`,`ds3231`) = Makefile targets = component.mk `.o`; glue `*_dev.{h,c}` consistent across header/impl/esp-gate. `bh1750_parse(raw[2], uint32_t*)`, `sht3x_parse(raw[6], int32_t*, int32_t*)`, `ds3231_decode/encode/temp_mc` identical in header, test, impl, and glue callers. `i2c_bus_read`/`i2c_bus_write` signatures match Task 0 decl/def and the bh1750/sht3x glue callers. No left-shift of negative (SHT3x CRC on uint8 non-negative-int operands; conversions widen to int64; ds3231 BCD on uint8; ds3231_temp_mc uses signed `*` not shift). Vectors recomputed: bh1750 `0xFFFF`→54_612_500; sht3x `0xBEEF`→t 85523/h 74584; ds3231 `0x19,0x40`→25250, `0xFF,0x80`→−500.
