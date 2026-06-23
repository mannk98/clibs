# esp-libs Cycle 8 — L3 drivers `ds18b20` + `servo` + `hcsr04` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three L3 device drivers to `esp_libs` — `ds18b20` (1-Wire temp, reusing the L1 `crc8_maxim`), `servo` (PWM angle), `hcsr04` (ultrasonic) — each a host-tested pure part + hardware-deferred glue, then wire them into the component + compile-gate.

**Architecture:** Per driver, a **pure `.c`** (no SDK header → host-Unity-tested) + a **glue `.c`** (bit-bang/PWM SDK code → compile-gate only, runtime hardware-deferred). Tasks 1–3 are **independent** (each only creates its own dir + standalone-`cc`-verifies its pure test) — they do NOT touch shared files and do NOT commit. Task 4 is the single **integration** step (wire `Makefile`/`component.mk`/`esp-gate`, run 17-suite host gate + `strict` + esp-gate link, commit).

**Tech Stack:** portable C11 + vendored Unity (host); ESP8266_RTOS_SDK `driver/gpio.h`, `driver/pwm.h`, `rom/ets_sys.h` (`ets_delay_us`); L1 `crc.h` (`crc8_maxim`).

## Global Constraints

- **Tasks 1–3 scope rule:** each creates ONLY its own `esp-libs/<driver>/` dir (the pure `.{h,c}`, the glue `.{h,c}`, the `test_*.c`). It MUST NOT edit `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, or any other driver; it MUST NOT `git commit`. All wiring + commits are Task 4.
- **Standalone verify (Tasks 1–3, outputs to `/tmp`):** from `/Users/man.nk/git/clibs/esp-libs`, compile the pure test:
  - ds18b20: `cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ids18b20 -Icrc -o /tmp/test_ds18b20_parse ds18b20/test_ds18b20_parse.c ds18b20/ds18b20_parse.c crc/crc.c ../common/third_party/unity/unity.c && /tmp/test_ds18b20_parse`
  - servo: `cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Iservo -o /tmp/test_servo_duty servo/test_servo_duty.c servo/servo_duty.c ../common/third_party/unity/unity.c && /tmp/test_servo_duty`
  - hcsr04: `cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ihcsr04 -o /tmp/test_hcsr04_dist hcsr04/test_hcsr04_dist.c hcsr04/hcsr04_dist.c ../common/third_party/unity/unity.c && /tmp/test_hcsr04_dist`
- **Pure `.c` = no SDK header** (ds18b20_parse may include `crc.h`, which is itself pure). Glue `.c` includes SDK headers; SDK decls in the glue header. `CLIBS_*_H` guards; `esp_err_t` for glue, `int32_t`/`uint32_t`/`bool` for pure; NULL/edge guards; no malloc; no duplicated CRC (reuse `crc8_maxim`).
- **Build env for the esp-gate (Task 4), as ONE command:** `export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4`.
- **Host gate ends at 17 suites** (14 + ds18b20_parse, servo_duty, hcsr04_dist), all `0 Failures`; `make strict` → `strict: OK`. Don't touch existing libs / `common/` / `avr-libs/` / `esp-idf-template/`.
- **Commits (Task 4):** end each message with `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `ds18b20` — 1-Wire temperature (pure parse host-tested + bit-bang glue)

**Files:** Create `esp-libs/ds18b20/ds18b20_parse.h`, `esp-libs/ds18b20/ds18b20_parse.c`, `esp-libs/ds18b20/test_ds18b20_parse.c`, `esp-libs/ds18b20/ds18b20.h`, `esp-libs/ds18b20/ds18b20.c`. (No wiring/commit — Task 4.)

**Interfaces — Produces:**
```c
bool      ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc);   /* ds18b20_parse.h */
typedef struct { gpio_num_t pin; } ds18b20;                                 /* ds18b20.h */
esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin);
esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc);
```
**Consumes:** L1 `crc.h` (`crc8_maxim`); SDK `driver/gpio.h`, `rom/ets_sys.h`, `esp_err.h`.

- [ ] **Step 1: Write `esp-libs/ds18b20/test_ds18b20_parse.c`**

```c
#include "unity.h"
#include "ds18b20_parse.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

static void test_valid_85c(void)
{
    /* DS18B20 power-on default scratchpad; raw 0x0550 = 1360 -> 85000 mC */
    uint8_t s[9] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0};
    s[8] = crc8_maxim(s, 8);
    int32_t t = 0;
    TEST_ASSERT_TRUE(ds18b20_parse_temp(s, &t));
    TEST_ASSERT_EQUAL_INT32(85000, t);
}

static void test_negative_10c(void)
{
    /* raw 0xFF60 = -160 -> -10000 mC */
    uint8_t s[9] = {0x60, 0xFF, 0, 0, 0, 0, 0, 0, 0};
    s[8] = crc8_maxim(s, 8);
    int32_t t = 0;
    TEST_ASSERT_TRUE(ds18b20_parse_temp(s, &t));
    TEST_ASSERT_EQUAL_INT32(-10000, t);
}

static void test_bad_crc(void)
{
    uint8_t s[9] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0};
    s[8] = crc8_maxim(s, 8);
    s[0] ^= 0xFF;   /* corrupt data, CRC now mismatches */
    int32_t t = 0;
    TEST_ASSERT_FALSE(ds18b20_parse_temp(s, &t));
}

static void test_null(void)
{
    uint8_t s[9] = {0};
    int32_t t = 0;
    TEST_ASSERT_FALSE(ds18b20_parse_temp(NULL, &t));
    TEST_ASSERT_FALSE(ds18b20_parse_temp(s, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_85c);
    RUN_TEST(test_negative_10c);
    RUN_TEST(test_bad_crc);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/ds18b20/ds18b20_parse.h`**

```c
#ifndef CLIBS_DS18B20_PARSE_H
#define CLIBS_DS18B20_PARSE_H

#include <stdint.h>
#include <stdbool.h>

/* Validate the 9-byte DS18B20 scratchpad (crc8_maxim(scratch,8) == scratch[8]) and
 * convert the temperature (scratch[0]=LSB, scratch[1]=MSB; LSB = 1/16 C) to
 * milli-degrees C. Bad CRC / NULL -> false. Pure: no SDK headers (host-testable). */
bool ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc);

#endif /* CLIBS_DS18B20_PARSE_H */
```

- [ ] **Step 3: Run the standalone ds18b20 verify (Global Constraints) → expect FAIL** (`ds18b20_parse.c: No such file` / undefined).

- [ ] **Step 4: Write `esp-libs/ds18b20/ds18b20_parse.c`**

```c
#include "ds18b20_parse.h"

#include "crc.h"   /* L1 crc8_maxim */

bool ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc)
{
    if (scratch == NULL || temp_mc == NULL) {
        return false;
    }
    if (crc8_maxim(scratch, 8) != scratch[8]) {
        return false;
    }
    int16_t raw = (int16_t) ((uint16_t) scratch[1] << 8 | scratch[0]);
    *temp_mc = (int32_t) raw * 625 / 10;   /* LSB = 0.0625 C = 62.5 mC */
    return true;
}
```

- [ ] **Step 5: Run the standalone ds18b20 verify → expect PASS** (`4 Tests 0 Failures 0 Ignored` / `OK`).

- [ ] **Step 6: Write `esp-libs/ds18b20/ds18b20.h`**

```c
#ifndef CLIBS_DS18B20_H
#define CLIBS_DS18B20_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "ds18b20_parse.h"

typedef struct { gpio_num_t pin; } ds18b20;

esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin);
/* Single-drop 1-Wire read; fills milli-deg C. ESP_OK / ESP_ERR_TIMEOUT (no presence) /
 * ESP_ERR_INVALID_CRC (bad scratchpad). */
esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc);

#endif /* CLIBS_DS18B20_H */
```

- [ ] **Step 7: Write `esp-libs/ds18b20/ds18b20.c`**

```c
#include "ds18b20.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* 1-Wire on a single open-drain pin: drive low via OUTPUT, release via INPUT (pull-up). */
static void ow_low(gpio_num_t pin)     { gpio_set_direction(pin, GPIO_MODE_OUTPUT); gpio_set_level(pin, 0); }
static void ow_release(gpio_num_t pin) { gpio_set_direction(pin, GPIO_MODE_INPUT); }

static bool ow_reset(gpio_num_t pin)
{
    ow_low(pin);
    ets_delay_us(480);
    ow_release(pin);
    ets_delay_us(70);
    bool present = (gpio_get_level(pin) == 0);
    ets_delay_us(410);
    return present;
}

static void ow_write_bit(gpio_num_t pin, int bit)
{
    ow_low(pin);
    if (bit) {
        ets_delay_us(6);
        ow_release(pin);
        ets_delay_us(64);
    } else {
        ets_delay_us(60);
        ow_release(pin);
        ets_delay_us(10);
    }
}

static int ow_read_bit(gpio_num_t pin)
{
    ow_low(pin);
    ets_delay_us(6);
    ow_release(pin);
    ets_delay_us(9);
    int b = gpio_get_level(pin);
    ets_delay_us(55);
    return b;
}

static void ow_write_byte(gpio_num_t pin, uint8_t v)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(pin, v & 1);
        v >>= 1;
    }
}

static uint8_t ow_read_byte(gpio_num_t pin)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) {
        v >>= 1;
        if (ow_read_bit(pin)) {
            v |= 0x80;
        }
    }
    return v;
}

esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    ow_release(pin);
    return ESP_OK;
}

esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc)
{
    if (self == NULL || temp_mc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t pin = self->pin;
    if (!ow_reset(pin)) {
        return ESP_ERR_TIMEOUT;
    }
    ow_write_byte(pin, 0xCC);   /* skip ROM */
    ow_write_byte(pin, 0x44);   /* convert T */
    ets_delay_us(750000);       /* up to 750 ms for 12-bit; tune on hardware (prefer vTaskDelay) */
    if (!ow_reset(pin)) {
        return ESP_ERR_TIMEOUT;
    }
    ow_write_byte(pin, 0xCC);   /* skip ROM */
    ow_write_byte(pin, 0xBE);   /* read scratchpad */
    uint8_t scratch[9];
    for (int i = 0; i < 9; i++) {
        scratch[i] = ow_read_byte(pin);
    }
    if (!ds18b20_parse_temp(scratch, temp_mc)) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
```

---

### Task 2: `servo` — PWM angle (pure angle→duty host-tested + PWM glue)

**Files:** Create `esp-libs/servo/servo_duty.h`, `esp-libs/servo/servo_duty.c`, `esp-libs/servo/test_servo_duty.c`, `esp-libs/servo/servo.h`, `esp-libs/servo/servo.c`. (No wiring/commit — Task 4.)

**Interfaces — Produces:**
```c
uint32_t  servo_angle_to_duty(uint8_t angle);                  /* servo_duty.h */
typedef struct { uint8_t angle; bool started; } servo;         /* servo.h */
esp_err_t servo_init(servo *self, gpio_num_t pin);
esp_err_t servo_set(servo *self, uint8_t angle);
```
**Consumes:** SDK `driver/pwm.h`, `driver/gpio.h`, `esp_err.h`.

- [ ] **Step 1: Write `esp-libs/servo/test_servo_duty.c`**

```c
#include "unity.h"
#include "servo_duty.h"

void setUp(void) {}
void tearDown(void) {}

static void test_servo_duty(void)
{
    TEST_ASSERT_EQUAL_UINT32(1000, servo_angle_to_duty(0));
    TEST_ASSERT_EQUAL_UINT32(1500, servo_angle_to_duty(90));
    TEST_ASSERT_EQUAL_UINT32(2000, servo_angle_to_duty(180));
    TEST_ASSERT_EQUAL_UINT32(2000, servo_angle_to_duty(200));   /* clamp >180 */
    TEST_ASSERT_EQUAL_UINT32(1250, servo_angle_to_duty(45));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_servo_duty);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/servo/servo_duty.h`**

```c
#ifndef CLIBS_SERVO_DUTY_H
#define CLIBS_SERVO_DUTY_H

#include <stdint.h>

/* Map angle (clamped to 0..180) to a servo pulse width in microseconds (1000..2000).
 * Pure: no SDK headers (host-testable). */
uint32_t servo_angle_to_duty(uint8_t angle);

#endif /* CLIBS_SERVO_DUTY_H */
```

- [ ] **Step 3: Run the standalone servo verify (Global Constraints) → expect FAIL.**

- [ ] **Step 4: Write `esp-libs/servo/servo_duty.c`**

```c
#include "servo_duty.h"

uint32_t servo_angle_to_duty(uint8_t angle)
{
    if (angle > 180) {
        angle = 180;
    }
    return 1000u + (uint32_t) angle * 1000u / 180u;
}
```

- [ ] **Step 5: Run the standalone servo verify → expect PASS** (`1 Tests 0 Failures 0 Ignored` / `OK`).

- [ ] **Step 6: Write `esp-libs/servo/servo.h`**

```c
#ifndef CLIBS_SERVO_H
#define CLIBS_SERVO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "servo_duty.h"

/* Software-PWM servo at 50 Hz (period 20000 us). NOTE: the SDK PWM is one global
 * subsystem (channel 0) — servo and pwm_dimmer cannot both run. */
typedef struct { uint8_t angle; bool started; } servo;

esp_err_t servo_init(servo *self, gpio_num_t pin);
esp_err_t servo_set(servo *self, uint8_t angle);

#endif /* CLIBS_SERVO_H */
```

- [ ] **Step 7: Write `esp-libs/servo/servo.c`**

```c
#include "servo.h"

#include "driver/pwm.h"

#define SERVO_PERIOD_US 20000

esp_err_t servo_init(servo *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->angle = 90;
    self->started = false;
    uint32_t pins[1]   = { (uint32_t) pin };
    uint32_t duties[1] = { servo_angle_to_duty(90) };
    esp_err_t err = pwm_init(SERVO_PERIOD_US, duties, 1, pins);
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();
    if (err != ESP_OK) {
        return err;
    }
    self->started = true;
    return ESP_OK;
}

esp_err_t servo_set(servo *self, uint8_t angle)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = pwm_set_duty(0, servo_angle_to_duty(angle));
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();
    if (err == ESP_OK) {
        self->angle = angle;
    }
    return err;
}
```

---

### Task 3: `hcsr04` — ultrasonic (pure us→mm host-tested + echo glue)

**Files:** Create `esp-libs/hcsr04/hcsr04_dist.h`, `esp-libs/hcsr04/hcsr04_dist.c`, `esp-libs/hcsr04/test_hcsr04_dist.c`, `esp-libs/hcsr04/hcsr04.h`, `esp-libs/hcsr04/hcsr04.c`. (No wiring/commit — Task 4.)

**Interfaces — Produces:**
```c
uint32_t  hcsr04_us_to_mm(uint32_t echo_us);                              /* hcsr04_dist.h */
typedef struct { gpio_num_t trig; gpio_num_t echo; } hcsr04;             /* hcsr04.h */
esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo);
esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm);
```
**Consumes:** SDK `driver/gpio.h`, `rom/ets_sys.h`, `esp_err.h`.

- [ ] **Step 1: Write `esp-libs/hcsr04/test_hcsr04_dist.c`**

```c
#include "unity.h"
#include "hcsr04_dist.h"

void setUp(void) {}
void tearDown(void) {}

static void test_dist(void)
{
    TEST_ASSERT_EQUAL_UINT32(0,    hcsr04_us_to_mm(0));
    TEST_ASSERT_EQUAL_UINT32(994,  hcsr04_us_to_mm(5800));
    TEST_ASSERT_EQUAL_UINT32(9,    hcsr04_us_to_mm(58));
    TEST_ASSERT_EQUAL_UINT32(6517, hcsr04_us_to_mm(38000));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dist);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/hcsr04/hcsr04_dist.h`**

```c
#ifndef CLIBS_HCSR04_DIST_H
#define CLIBS_HCSR04_DIST_H

#include <stdint.h>

/* Convert echo-high duration (us) to distance in mm: us * 343 / 2000 (speed of
 * sound ~343 m/s, round-trip halved). Valid echo <= ~38000 us (no uint32 overflow).
 * Pure: no SDK headers (host-testable). */
uint32_t hcsr04_us_to_mm(uint32_t echo_us);

#endif /* CLIBS_HCSR04_DIST_H */
```

- [ ] **Step 3: Run the standalone hcsr04 verify (Global Constraints) → expect FAIL.**

- [ ] **Step 4: Write `esp-libs/hcsr04/hcsr04_dist.c`**

```c
#include "hcsr04_dist.h"

uint32_t hcsr04_us_to_mm(uint32_t echo_us)
{
    return echo_us * 343u / 2000u;
}
```

- [ ] **Step 5: Run the standalone hcsr04 verify → expect PASS** (`1 Tests 0 Failures 0 Ignored` / `OK`).

- [ ] **Step 6: Write `esp-libs/hcsr04/hcsr04.h`**

```c
#ifndef CLIBS_HCSR04_H
#define CLIBS_HCSR04_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "hcsr04_dist.h"

typedef struct { gpio_num_t trig; gpio_num_t echo; } hcsr04;

esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo);
/* Trigger + measure echo; fills mm. ESP_OK / ESP_ERR_TIMEOUT (no echo within bound). */
esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm);

#endif /* CLIBS_HCSR04_H */
```

- [ ] **Step 7: Write `esp-libs/hcsr04/hcsr04.c`**

```c
#include "hcsr04.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

#define HCSR04_TIMEOUT_US 40000   /* ~6.8 m round trip */

esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->trig = trig;
    self->echo = echo;
    gpio_set_direction(trig, GPIO_MODE_OUTPUT);
    gpio_set_level(trig, 0);
    gpio_set_direction(echo, GPIO_MODE_INPUT);
    return ESP_OK;
}

esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm)
{
    if (self == NULL || mm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t trig = self->trig;
    gpio_num_t echo = self->echo;

    /* 10 us trigger pulse */
    gpio_set_level(trig, 1);
    ets_delay_us(10);
    gpio_set_level(trig, 0);

    /* wait for the echo rising edge */
    uint32_t waited = 0;
    while (gpio_get_level(echo) == 0) {
        if (waited >= HCSR04_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
        waited++;
    }
    /* measure the echo-high duration */
    uint32_t dur = 0;
    while (gpio_get_level(echo) != 0) {
        if (dur >= HCSR04_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
        dur++;
    }
    *mm = hcsr04_us_to_mm(dur);
    return ESP_OK;
}
```

---

### Task 4: Integration — wire all 3 into Makefile + component.mk + esp-gate, run gates, commit + README

**Files:** Modify `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`. (Depends on Tasks 1–3 being present on disk.)

- [ ] **Step 1: Add the 3 host-test targets to `esp-libs/Makefile`** — three edits:

(a) Replace the `.PHONY` and `test:` lines with (append the 3 new names):
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist
```
(b) Add these three target blocks after the `throttle:` target block (note ds18b20_parse links `crc/crc.c` with `-Icrc`):
```makefile
ds18b20_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ids18b20 -Icrc -o $(BUILD)/test_ds18b20_parse \
	    ds18b20/test_ds18b20_parse.c ds18b20/ds18b20_parse.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_ds18b20_parse

servo_duty: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iservo -o $(BUILD)/test_servo_duty \
	    servo/test_servo_duty.c servo/servo_duty.c $(UNITY)/unity.c
	./$(BUILD)/test_servo_duty

hcsr04_dist: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihcsr04 -o $(BUILD)/test_hcsr04_dist \
	    hcsr04/test_hcsr04_dist.c hcsr04/hcsr04_dist.c $(UNITY)/unity.c
	./$(BUILD)/test_hcsr04_dist
```
(c) Add these three lines to the `strict:` recipe (before `@echo "strict: OK"`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ids18b20 -Icrc -c ds18b20/ds18b20_parse.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iservo       -c servo/servo_duty.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihcsr04      -c hcsr04/hcsr04_dist.c     -o /dev/null
```

- [ ] **Step 2: Wire the 3 dirs into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o \
                  hysteresis/hysteresis.o crc/crc.o map_range/map_range.o \
                  median/median.o throttle/throttle.o \
                  adc_a0/adc_a0.o i2c_bus/i2c_bus.o spi_bus/spi_bus.o \
                  ds18b20/ds18b20_parse.o ds18b20/ds18b20.o \
                  servo/servo_duty.o servo/servo.o \
                  hcsr04/hcsr04_dist.o hcsr04/hcsr04.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 3: Touch the 3 drivers in `esp-gate/main/main.c`** — add after `#include "spi_bus.h"`:

```c
#include "ds18b20.h"
#include "servo.h"
#include "hcsr04.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* L3 sensors compile-gate. */
    ds18b20 ow;
    (void) ds18b20_init(&ow, GPIO_NUM_2);
    int32_t owt = 0;
    (void) ds18b20_read(&ow, &owt);
    servo sv;
    (void) servo_init(&sv, GPIO_NUM_14);
    (void) servo_set(&sv, 90);
    hcsr04 us;
    (void) hcsr04_init(&us, GPIO_NUM_12, GPIO_NUM_13);
    uint32_t usmm = 0;
    (void) hcsr04_read(&us, &usmm);
    ESP_LOGI(TAG, "ds18b20=%d hcsr04=%u", (int) owt, (unsigned) usmm);
```

- [ ] **Step 4: Run the host gate**

```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: **17** `... 0 Failures ... OK` lines + `strict: OK`.

- [ ] **Step 5: Run the esp-gate compile-gate**

```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make defconfig >/dev/null && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`. Then confirm symbols:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; cd /Users/man.nk/git/clibs/esp-gate && xtensa-lx106-elf-nm build/esp_libs_gate.elf | grep -E " T (ds18b20_read|servo_set|hcsr04_read)"
```
Expected: all three as `T`.

- [ ] **Step 6: Commit the drivers + wiring**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/ds18b20/ esp-libs/servo/ esp-libs/hcsr04/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs): L3 drivers ds18b20 + servo + hcsr04 (host-tested pure + gated glue)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

- [ ] **Step 7: Update `esp-libs/README.md`** — two edits (read the current README first to anchor):

(a) Append `, ds18b20, servo, hcsr04` to the Layer-3 intro bullet's lib list (the `**Layer 3 — device drivers (this set):**` line listing `relay`, `button`, `dht`, `pwm_dimmer`).

(b) Insert these subsections immediately before the `### Compile-gate` subsection:
```markdown
### `ds18b20` — 1-Wire temperature

- `ds18b20_parse_temp(scratch, &temp_mc)` (pure, host-tested): validate the 9-byte
  scratchpad via `crc8_maxim` and convert to milli-°C.
- `ds18b20_init(&d, pin)` then `ds18b20_read(&d, &temp_mc)` bit-bangs a single-drop
  1-Wire read. Timing is a first cut — tune on hardware.

### `servo` — PWM servo

- `servo_angle_to_duty(angle)` (pure, host-tested): 0–180° → 1000–2000 µs pulse.
- `servo_init(&s, pin)` (50 Hz) then `servo_set(&s, angle)`. Shares the single global
  PWM with `pwm_dimmer` — only one at a time.

### `hcsr04` — ultrasonic distance

- `hcsr04_us_to_mm(echo_us)` (pure, host-tested): echo time → mm.
- `hcsr04_init(&h, trig, echo)` then `hcsr04_read(&h, &mm)` (trigger + bounded echo
  measure → `ESP_ERR_TIMEOUT` on no echo). Timing tuned on hardware.
```

- [ ] **Step 8: Commit the docs**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document L3 ds18b20 + servo + hcsr04

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:** ds18b20 pure parse (reuses crc) + 1-Wire glue → Task 1; servo pure angle→duty + PWM glue → Task 2; hcsr04 pure us→mm + echo glue → Task 3; Makefile (17 suites) + component.mk (6 .o) + esp-gate + README → Task 4. All spec modules + infra. ✓

**Placeholder scan:** No TBD/TODO; every file has complete code; commands have expected output. ✓

**Type consistency:** Each driver's signatures match across its pure header/impl/test, glue header/impl, and the Task-4 esp-gate touch (`ds18b20_parse_temp`/`ds18b20_init`/`ds18b20_read`, `servo_angle_to_duty`/`servo_init`/`servo_set`, `hcsr04_us_to_mm`/`hcsr04_init`/`hcsr04_read`). `component.mk` appends 3 dirs + 6 `.o` to the cycle-7 list; `Makefile` `.PHONY`/`test:` add the 3 suites; `strict` covers the 3 pure `.c` (ds18b20_parse with `-Icrc`). `ds18b20_parse.c` includes `crc.h` (the host target + strict line add `-Icrc` and link `crc/crc.c`). ✓

**Test-vector check:** ds18b20 raw 0x0550=1360 → 1360*625/10=85000 mC; raw 0xFF60=−160 → −10000 mC; bad-CRC (flip byte, keep CRC) → false. servo 0→1000, 90→1500, 180→2000, 200→clamp 2000, 45→1250. hcsr04 5800→994, 58→9, 38000→6517, 0→0. ✓
