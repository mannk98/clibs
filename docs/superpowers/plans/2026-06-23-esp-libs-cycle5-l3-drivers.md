# esp-libs Cycle 5 — Layer-3 drivers `relay` + `button` + `dht` + `pwm_dimmer` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four Layer-3 device drivers to the `esp_libs` component — `relay`, `button`, `dht`, `pwm_dimmer` — with each driver's pure logic host-Unity-tested and its GPIO/PWM/timing glue verified by the `esp-gate` link. Hardware run is deferred.

**Architecture:** Same split as cycle 4 — a **pure `.c`** (no SDK header, host-compilable + Unity-tested) holds the testable logic (`relay_level`, `dht_parse`, `dimmer_duty`); a **separate SDK-glue `.c`** holds the `driver/gpio.h`/`driver/pwm.h`/`rom/ets_sys.h` code, compiled only by the component and exercised by symbol-touches in `esp-gate/main/main.c`. `button` has no pure part of its own (it reuses the L1 `debounce` FSM), so it is one compile-gate-only file. Per-task GREEN: host `make test` passes AND `esp-gate` links.

**Tech Stack:** ESP8266_RTOS_SDK v3.4 (xtensa-lx106-elf gcc via Rosetta), legacy `make`; SDK includes `driver/gpio.h` (`gpio_set_direction`/`gpio_set_level`/`gpio_get_level`/`gpio_set_pull_mode`, `GPIO_MODE_OUTPUT`/`GPIO_MODE_INPUT`, `GPIO_PULLUP_ONLY`/`GPIO_FLOATING`, `gpio_num_t`), `driver/pwm.h` (`pwm_init`/`pwm_set_duty`/`pwm_start`), `rom/ets_sys.h` (`ets_delay_us`); host `cc` + vendored Unity; L1 `debounce`.

## Global Constraints

- **Build env (run once per shell before any ESP gate build, as ONE command — `export`s do NOT persist across separate Bash calls):**
  ```sh
  export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4
  ```
  First ESP build takes a few minutes. Run `make defconfig` once if `sdkconfig` is missing.
- **Two test postures + a hardware caveat:** pure logic (`relay_level`, `dht_parse`, `dimmer_duty`) is **host-Unity-tested** (real RED→GREEN). GPIO/PWM/timing glue (`relay.c`, `button.c`, `dht.c`, `pwm_dimmer.c`) is **compile-gate only** — do NOT write host tests for it, do NOT fabricate empty tests. **Running on hardware is out of scope this cycle** (the user will run one pass + fix later, especially `dht`'s bit-bang timing).
- **Pure `.c` files must NOT include any SDK header** (`esp_err.h`, `driver/*.h`, `rom/*.h`) so host `cc` compiles them. The SDK return types/decls go in a *separate* SDK-only header.
- **clibs conventions:** header guards `CLIBS_<NAME>_H`; `esp_err_t` returns for SDK glue, `int`/`uint32_t`/`bool` for pure helpers; NULL-arg guards; caller-provided transparent struct storage, no malloc; `button` reuses L1 `debounce` (no duplicated FSM).
- **`component.mk` excludes `test_*.c`** (explicit `COMPONENT_OBJS`); keep the existing `COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip` line unchanged (gpio/pwm are in the core `esp8266` component — no new require expected; the gate is the check).
- **Host gate ends this cycle at 9 suites** (backoff, debounce, filter, mqtt_topic, device_id, json_build, relay_level, dht_parse, dimmer_duty), all `0 Failures`; `make strict` → `strict: OK`. Do not touch L1/L2 sources, `common/`, `avr-libs/`, `esp-idf-template/`.
- **Commits:** end each message with the trailer `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `relay` — GPIO output actuator (pure level map host-tested + SDK glue)

**Files:**
- Create: `esp-libs/relay/relay_level.h`, `esp-libs/relay/relay_level.c`, `esp-libs/relay/relay.h`, `esp-libs/relay/relay.c`, `esp-libs/relay/test_relay_level.c`
- Modify: `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`

**Interfaces:**
- Consumes: `driver/gpio.h` (`gpio_num_t`, `gpio_set_direction`, `gpio_set_level`, `GPIO_MODE_OUTPUT`), `esp_err.h`.
- Produces:
  ```c
  int       relay_level(bool on, bool active_low);                       /* relay_level.h */
  typedef struct { gpio_num_t pin; bool active_low; bool on; } relay;    /* relay.h */
  esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low);
  esp_err_t relay_set(relay *self, bool on);
  esp_err_t relay_toggle(relay *self);
  bool      relay_is_on(const relay *self);
  ```

- [ ] **Step 1: Write the failing host test `esp-libs/relay/test_relay_level.c`**

```c
#include "unity.h"
#include "relay_level.h"

void setUp(void) {}
void tearDown(void) {}

static void test_active_high(void)
{
    TEST_ASSERT_EQUAL_INT(1, relay_level(true, false));
    TEST_ASSERT_EQUAL_INT(0, relay_level(false, false));
}

static void test_active_low(void)
{
    TEST_ASSERT_EQUAL_INT(0, relay_level(true, true));
    TEST_ASSERT_EQUAL_INT(1, relay_level(false, true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_high);
    RUN_TEST(test_active_low);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/relay/relay_level.h`**

```c
#ifndef CLIBS_RELAY_LEVEL_H
#define CLIBS_RELAY_LEVEL_H

#include <stdbool.h>

/* GPIO level (0/1) to drive for the desired on-state; active_low inverts.
 * Pure: no SDK headers (host-testable). */
int relay_level(bool on, bool active_low);

#endif /* CLIBS_RELAY_LEVEL_H */
```

- [ ] **Step 3: Run the test to verify it FAILS**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Irelay -o build/test_relay_level relay/test_relay_level.c relay/relay_level.c ../common/third_party/unity/unity.c
```
Expected: FAIL — `relay/relay_level.c: No such file` (or undefined `relay_level`).

- [ ] **Step 4: Write `esp-libs/relay/relay_level.c`**

```c
#include "relay_level.h"

int relay_level(bool on, bool active_low)
{
    if (active_low) {
        return on ? 0 : 1;
    }
    return on ? 1 : 0;
}
```

- [ ] **Step 5: Run the test to verify it PASSES**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make relay_level
```
(After Step 6 wires the Makefile target. If running before Step 6, use the explicit `cc` line from Step 3 then `./build/test_relay_level`.) Expected: `2 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Write `esp-libs/relay/relay.h`**

```c
#ifndef CLIBS_RELAY_H
#define CLIBS_RELAY_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "relay_level.h"

typedef struct {
    gpio_num_t pin;
    bool       active_low;
    bool       on;
} relay;

/* Configure `pin` as a push-pull output and drive the relay OFF. */
esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low);
esp_err_t relay_set(relay *self, bool on);
esp_err_t relay_toggle(relay *self);
bool      relay_is_on(const relay *self);

#endif /* CLIBS_RELAY_H */
```

- [ ] **Step 7: Write `esp-libs/relay/relay.c`**

```c
#include "relay.h"

esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    self->active_low = active_low;
    self->on = false;
    esp_err_t err = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) {
        return err;
    }
    return relay_set(self, false);
}

esp_err_t relay_set(relay *self, bool on)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = gpio_set_level(self->pin, (uint32_t) relay_level(on, self->active_low));
    if (err == ESP_OK) {
        self->on = on;
    }
    return err;
}

esp_err_t relay_toggle(relay *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return relay_set(self, !self->on);
}

bool relay_is_on(const relay *self)
{
    return (self != NULL) && self->on;
}
```

- [ ] **Step 8: Wire the `relay_level` host target into `esp-libs/Makefile`** — three edits:

(a) Add `relay_level` to `.PHONY` and `test:`:
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level
```
(b) Add this target after the `json_build:` target block:
```makefile
relay_level: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irelay -o $(BUILD)/test_relay_level \
	    relay/test_relay_level.c relay/relay_level.c $(UNITY)/unity.c
	./$(BUILD)/test_relay_level
```
(c) Add to the `strict:` recipe (before `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Irelay -c relay/relay_level.c -o /dev/null
```

- [ ] **Step 9: Wire `relay` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 10: Touch `relay` in `esp-gate/main/main.c`** — add after `#include "mdns_node.h"`:

```c
#include "relay.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* relay compile-gate. */
    relay r;
    (void) relay_init(&r, GPIO_NUM_5, false);
    (void) relay_set(&r, true);
    (void) relay_toggle(&r);
    ESP_LOGI(TAG, "relay on=%d", (int) relay_is_on(&r));
```

- [ ] **Step 11: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: SEVEN `... 0 Failures ... OK` lines (incl. relay_level) + `strict: OK`.

ESP gate (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make defconfig >/dev/null && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 12: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/relay/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/relay): GPIO output actuator (host-tested level map + SDK glue)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: `button` — GPIO input over L1 `debounce` (compile-gate only)

**Files:**
- Create: `esp-libs/button/button.h`, `esp-libs/button/button.c`
- Modify: `esp-libs/component.mk`, `esp-gate/main/main.c`
- Test: none (logic is the L1 `debounce`, already tested). GREEN = `esp-gate` links + host stays 7 suites.

**Interfaces:**
- Consumes: `driver/gpio.h` (`gpio_set_direction`, `gpio_get_level`, `gpio_set_pull_mode`, `GPIO_MODE_INPUT`, `GPIO_PULLUP_ONLY`, `GPIO_FLOATING`), `debounce.h` (`debounce`, `debounce_init`, `debounce_update`, `debounce_level`, `DEBOUNCE_RISING`, `DEBOUNCE_FALLING`), `esp_err.h`.
- Produces:
  ```c
  typedef enum { BUTTON_NONE, BUTTON_PRESSED, BUTTON_RELEASED } button_event;
  typedef struct { gpio_num_t pin; bool active_low; debounce db; } button;
  esp_err_t    button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold);
  button_event button_poll(button *self);
  bool         button_is_pressed(const button *self);
  ```

- [ ] **Step 1: Write `esp-libs/button/button.h`**

```c
#ifndef CLIBS_BUTTON_H
#define CLIBS_BUTTON_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "debounce.h"   /* L1 debounce FSM */

typedef enum { BUTTON_NONE, BUTTON_PRESSED, BUTTON_RELEASED } button_event;

typedef struct {
    gpio_num_t pin;
    bool       active_low;
    debounce   db;
} button;

/* Configure `pin` as input (pull-up if active_low) and init the debounce FSM. */
esp_err_t button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold);

/* Sample the pin once (call at a fixed interval). Returns a debounced edge. */
button_event button_poll(button *self);

/* Current debounced pressed state. */
bool button_is_pressed(const button *self);

#endif /* CLIBS_BUTTON_H */
```

- [ ] **Step 2: Write `esp-libs/button/button.c`**

```c
#include "button.h"

esp_err_t button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    self->active_low = active_low;
    esp_err_t err = gpio_set_direction(pin, GPIO_MODE_INPUT);
    if (err != ESP_OK) {
        return err;
    }
    /* ESP8266 has an internal pull-up only (no pull-down on most pins); for an
       active-high button add an external pull-down resistor. */
    err = gpio_set_pull_mode(pin, active_low ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
    if (err != ESP_OK) {
        return err;
    }
    debounce_init(&self->db, threshold ? threshold : 1, 0);
    return ESP_OK;
}

button_event button_poll(button *self)
{
    if (self == NULL) {
        return BUTTON_NONE;
    }
    int level = gpio_get_level(self->pin);
    uint8_t pressed_raw = self->active_low ? (level == 0) : (level != 0);
    debounce_edge edge = debounce_update(&self->db, pressed_raw);
    if (edge == DEBOUNCE_RISING) {
        return BUTTON_PRESSED;
    }
    if (edge == DEBOUNCE_FALLING) {
        return BUTTON_RELEASED;
    }
    return BUTTON_NONE;
}

bool button_is_pressed(const button *self)
{
    if (self == NULL) {
        return false;
    }
    return debounce_level(&self->db) != 0;
}
```

- [ ] **Step 3: Wire `button` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 4: Touch `button` in `esp-gate/main/main.c`** — add after `#include "relay.h"`:

```c
#include "button.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* button compile-gate. */
    button btn;
    (void) button_init(&btn, GPIO_NUM_4, true, 3);
    (void) button_poll(&btn);
    ESP_LOGI(TAG, "btn pressed=%d", (int) button_is_pressed(&btn));
```

- [ ] **Step 5: Run both gates**

Host (still 7 suites — button adds no host test):
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `7` and `strict: OK`.

ESP gate:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 6: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/button/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/button): GPIO input over L1 debounce (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: `dht` — DHT22/DHT11 (pure frame parse host-tested + bit-bang glue)

**Files:**
- Create: `esp-libs/dht/dht_parse.h`, `esp-libs/dht/dht_parse.c`, `esp-libs/dht/dht.h`, `esp-libs/dht/dht.c`, `esp-libs/dht/test_dht_parse.c`
- Modify: `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`

**Interfaces:**
- Consumes: `driver/gpio.h`, `rom/ets_sys.h` (`ets_delay_us`), `esp_err.h`.
- Produces:
  ```c
  bool      dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct); /* dht_parse.h */
  typedef struct { gpio_num_t pin; } dht;                                            /* dht.h */
  esp_err_t dht_init(dht *self, gpio_num_t pin);
  esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct);
  ```

- [ ] **Step 1: Write the failing host test `esp-libs/dht/test_dht_parse.c`**

```c
#include "unity.h"
#include "dht_parse.h"

void setUp(void) {}
void tearDown(void) {}

static void test_valid_frame(void)
{
    /* humidity 0x0292=658 (65.8%), temp 0x014A=330 (33.0C), checksum 0xDF */
    uint8_t f[5] = {0x02, 0x92, 0x01, 0x4A, 0xDF};
    int16_t t = 0, h = 0;
    TEST_ASSERT_TRUE(dht_parse(f, &t, &h));
    TEST_ASSERT_EQUAL_INT16(658, h);
    TEST_ASSERT_EQUAL_INT16(330, t);
}

static void test_negative_temp(void)
{
    /* temp byte 0x80 sets the sign bit: -(0x0014)= -20 (-2.0C); checksum 0x28 */
    uint8_t f[5] = {0x02, 0x92, 0x80, 0x14, 0x28};
    int16_t t = 0, h = 0;
    TEST_ASSERT_TRUE(dht_parse(f, &t, &h));
    TEST_ASSERT_EQUAL_INT16(-20, t);
    TEST_ASSERT_EQUAL_INT16(658, h);
}

static void test_bad_checksum(void)
{
    uint8_t f[5] = {0x02, 0x92, 0x01, 0x4A, 0x00};   /* wrong checksum */
    int16_t t = 0, h = 0;
    TEST_ASSERT_FALSE(dht_parse(f, &t, &h));
}

static void test_null(void)
{
    uint8_t f[5] = {0};
    int16_t t = 0, h = 0;
    TEST_ASSERT_FALSE(dht_parse(NULL, &t, &h));
    TEST_ASSERT_FALSE(dht_parse(f, NULL, &h));
    TEST_ASSERT_FALSE(dht_parse(f, &t, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_frame);
    RUN_TEST(test_negative_temp);
    RUN_TEST(test_bad_checksum);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/dht/dht_parse.h`**

```c
#ifndef CLIBS_DHT_PARSE_H
#define CLIBS_DHT_PARSE_H

#include <stdint.h>
#include <stdbool.h>

/* Parse a 5-byte DHT frame. Returns true + fills outputs on a valid checksum.
 *   humidity    = (b0<<8 | b1)            -> deci-%RH  (e.g. 658 = 65.8%)
 *   temperature = ((b2&0x7F)<<8 | b3)     -> deci-C, negated if b2&0x80
 *   checksum    = (b0+b1+b2+b3) & 0xFF == b4
 * NULL arg / bad checksum -> false. Pure: no SDK headers (host-testable). */
bool dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct);

#endif /* CLIBS_DHT_PARSE_H */
```

- [ ] **Step 3: Run the test to verify it FAILS**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Idht -o build/test_dht_parse dht/test_dht_parse.c dht/dht_parse.c ../common/third_party/unity/unity.c
```
Expected: FAIL — `dht/dht_parse.c: No such file` (or undefined `dht_parse`).

- [ ] **Step 4: Write `esp-libs/dht/dht_parse.c`**

```c
#include "dht_parse.h"

bool dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct)
{
    if (frame == NULL || temp_dc == NULL || humi_dpct == NULL) {
        return false;
    }
    uint8_t sum = (uint8_t) (frame[0] + frame[1] + frame[2] + frame[3]);
    if (sum != frame[4]) {
        return false;
    }
    *humi_dpct = (int16_t) (((uint16_t) frame[0] << 8) | frame[1]);
    int16_t t = (int16_t) (((uint16_t) (frame[2] & 0x7F) << 8) | frame[3]);
    if (frame[2] & 0x80) {
        t = (int16_t) (-t);
    }
    *temp_dc = t;
    return true;
}
```

- [ ] **Step 5: Run the test to verify it PASSES**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make dht_parse
```
(After Step 8 wires the target; otherwise use the Step 3 `cc` line + `./build/test_dht_parse`.) Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Write `esp-libs/dht/dht.h`**

```c
#ifndef CLIBS_DHT_H
#define CLIBS_DHT_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "dht_parse.h"

typedef struct {
    gpio_num_t pin;
} dht;

esp_err_t dht_init(dht *self, gpio_num_t pin);

/* Bit-bang one read; on success fills deci-units. Returns ESP_OK,
 * ESP_ERR_TIMEOUT (no sensor response), or ESP_ERR_INVALID_CRC (bad checksum). */
esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct);

#endif /* CLIBS_DHT_H */
```

- [ ] **Step 7: Write `esp-libs/dht/dht.c`**

```c
#include "dht.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* Spin until `pin` reaches `level`, up to `timeout_us`. Returns elapsed us, or -1. */
static int wait_level(gpio_num_t pin, int level, int timeout_us)
{
    int us = 0;
    while (gpio_get_level(pin) != level) {
        if (us >= timeout_us) {
            return -1;
        }
        ets_delay_us(1);
        us++;
    }
    return us;
}

esp_err_t dht_init(dht *self, gpio_num_t pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->pin = pin;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 1);   /* idle high */
    return ESP_OK;
}

esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct)
{
    if (self == NULL || temp_dc == NULL || humi_dpct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_num_t pin = self->pin;
    uint8_t bytes[5] = {0};

    /* Start signal: hold low ~20 ms (covers DHT11/DHT22), release, then read. */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(20000);
    gpio_set_level(pin, 1);
    ets_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    /* Sensor response: ~80us low then ~80us high. */
    if (wait_level(pin, 0, 100) < 0) { return ESP_ERR_TIMEOUT; }
    if (wait_level(pin, 1, 100) < 0) { return ESP_ERR_TIMEOUT; }
    if (wait_level(pin, 0, 100) < 0) { return ESP_ERR_TIMEOUT; }

    /* 40 bits: each = ~50us low then a high pulse; long high (>~40us) = 1.
     * NOTE: this 1us-granularity timing is a first cut — tune on hardware. */
    for (int i = 0; i < 40; i++) {
        if (wait_level(pin, 1, 100) < 0) { return ESP_ERR_TIMEOUT; }
        int high_us = wait_level(pin, 0, 100);
        if (high_us < 0) { return ESP_ERR_TIMEOUT; }
        bytes[i / 8] = (uint8_t) (bytes[i / 8] << 1);
        if (high_us > 40) {
            bytes[i / 8] |= 1;
        }
    }

    if (!dht_parse(bytes, temp_dc, humi_dpct)) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
```

- [ ] **Step 8: Wire the `dht_parse` host target into `esp-libs/Makefile`** — three edits:

(a) Add `dht_parse` to `.PHONY` and `test:`:
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse
```
(b) Add this target after the `relay_level:` target block:
```makefile
dht_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idht -o $(BUILD)/test_dht_parse \
	    dht/test_dht_parse.c dht/dht_parse.c $(UNITY)/unity.c
	./$(BUILD)/test_dht_parse
```
(c) Add to the `strict:` recipe (before `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Idht -c dht/dht_parse.c -o /dev/null
```

- [ ] **Step 9: Wire `dht` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 10: Touch `dht` in `esp-gate/main/main.c`** — add after `#include "button.h"`:

```c
#include "dht.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* dht compile-gate. */
    dht dh;
    (void) dht_init(&dh, GPIO_NUM_2);
    int16_t dht_t = 0, dht_h = 0;
    (void) dht_read(&dh, &dht_t, &dht_h);
    ESP_LOGI(TAG, "dht t=%d h=%d", (int) dht_t, (int) dht_h);
```

- [ ] **Step 11: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: EIGHT `... 0 Failures ... OK` lines + `strict: OK`.

ESP gate:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 12: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/dht/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/dht): DHT22/11 driver (host-tested frame parse + bit-bang read)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: `pwm_dimmer` — software PWM dimmer (pure duty map host-tested + SDK glue)

**Files:**
- Create: `esp-libs/pwm_dimmer/dimmer_duty.h`, `esp-libs/pwm_dimmer/dimmer_duty.c`, `esp-libs/pwm_dimmer/pwm_dimmer.h`, `esp-libs/pwm_dimmer/pwm_dimmer.c`, `esp-libs/pwm_dimmer/test_dimmer_duty.c`
- Modify: `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`

**Interfaces:**
- Consumes: `driver/pwm.h` (`pwm_init`, `pwm_set_duty`, `pwm_start`), `driver/gpio.h` (`gpio_num_t`), `esp_err.h`.
- Produces:
  ```c
  uint32_t  dimmer_duty(uint8_t percent, uint32_t max_duty);                       /* dimmer_duty.h */
  typedef struct { uint32_t period; uint32_t max_duty; uint8_t percent; bool started; } pwm_dimmer;
  esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us);
  esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent);
  ```

- [ ] **Step 1: Write the failing host test `esp-libs/pwm_dimmer/test_dimmer_duty.c`**

```c
#include "unity.h"
#include "dimmer_duty.h"

void setUp(void) {}
void tearDown(void) {}

static void test_zero(void)  { TEST_ASSERT_EQUAL_UINT32(0,    dimmer_duty(0,   1000)); }
static void test_half(void)  { TEST_ASSERT_EQUAL_UINT32(500,  dimmer_duty(50,  1000)); }
static void test_full(void)  { TEST_ASSERT_EQUAL_UINT32(1000, dimmer_duty(100, 1000)); }
static void test_clamp(void) { TEST_ASSERT_EQUAL_UINT32(1000, dimmer_duty(255, 1000)); }
static void test_round(void) { TEST_ASSERT_EQUAL_UINT32(33,   dimmer_duty(33,  100));  }

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_zero);
    RUN_TEST(test_half);
    RUN_TEST(test_full);
    RUN_TEST(test_clamp);
    RUN_TEST(test_round);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/pwm_dimmer/dimmer_duty.h`**

```c
#ifndef CLIBS_DIMMER_DUTY_H
#define CLIBS_DIMMER_DUTY_H

#include <stdint.h>

/* Map percent (clamped to 0..100) to a duty in [0, max_duty], rounded to nearest.
 * Pure: no SDK headers (host-testable). */
uint32_t dimmer_duty(uint8_t percent, uint32_t max_duty);

#endif /* CLIBS_DIMMER_DUTY_H */
```

- [ ] **Step 3: Run the test to verify it FAILS**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ipwm_dimmer -o build/test_dimmer_duty pwm_dimmer/test_dimmer_duty.c pwm_dimmer/dimmer_duty.c ../common/third_party/unity/unity.c
```
Expected: FAIL — `pwm_dimmer/dimmer_duty.c: No such file` (or undefined `dimmer_duty`).

- [ ] **Step 4: Write `esp-libs/pwm_dimmer/dimmer_duty.c`**

```c
#include "dimmer_duty.h"

uint32_t dimmer_duty(uint8_t percent, uint32_t max_duty)
{
    if (percent >= 100) {
        return max_duty;
    }
    return (uint32_t) (((uint64_t) max_duty * percent + 50) / 100);   /* rounded */
}
```

- [ ] **Step 5: Run the test to verify it PASSES**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make dimmer_duty
```
(After Step 8 wires the target; otherwise use the Step 3 `cc` line + `./build/test_dimmer_duty`.) Expected: `5 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Write `esp-libs/pwm_dimmer/pwm_dimmer.h`**

```c
#ifndef CLIBS_PWM_DIMMER_H
#define CLIBS_PWM_DIMMER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "dimmer_duty.h"

/* Single-dimmer wrapper over the SDK software PWM. NOTE: the SDK PWM is a single
 * global subsystem; this wrapper assumes ONE dimmer on channel 0. */
typedef struct {
    uint32_t period;
    uint32_t max_duty;
    uint8_t  percent;
    bool     started;
} pwm_dimmer;

esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us);
esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent);

#endif /* CLIBS_PWM_DIMMER_H */
```

- [ ] **Step 7: Write `esp-libs/pwm_dimmer/pwm_dimmer.c`**

```c
#include "pwm_dimmer.h"

#include "driver/pwm.h"

esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->period = period_us;
    self->max_duty = period_us;   /* duty is in the same units as the period */
    self->percent = 0;
    self->started = false;

    uint32_t pins[1]   = { (uint32_t) pin };
    uint32_t duties[1] = { 0 };
    esp_err_t err = pwm_init(period_us, duties, 1, pins);
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

esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = pwm_set_duty(0, dimmer_duty(percent, self->max_duty));
    if (err != ESP_OK) {
        return err;
    }
    err = pwm_start();   /* re-apply the new duty */
    if (err == ESP_OK) {
        self->percent = percent;
    }
    return err;
}
```

- [ ] **Step 8: Wire the `dimmer_duty` host target into `esp-libs/Makefile`** — three edits:

(a) Add `dimmer_duty` to `.PHONY` and `test:`:
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty
```
(b) Add this target after the `dht_parse:` target block:
```makefile
dimmer_duty: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ipwm_dimmer -o $(BUILD)/test_dimmer_duty \
	    pwm_dimmer/test_dimmer_duty.c pwm_dimmer/dimmer_duty.c $(UNITY)/unity.c
	./$(BUILD)/test_dimmer_duty
```
(c) Add to the `strict:` recipe (before `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ipwm_dimmer -c pwm_dimmer/dimmer_duty.c -o /dev/null
```

- [ ] **Step 9: Wire `pwm_dimmer` into `esp-libs/component.mk`** — replace the whole file with the FINAL version:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 10: Touch `pwm_dimmer` in `esp-gate/main/main.c`** — add after `#include "dht.h"`:

```c
#include "pwm_dimmer.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* pwm_dimmer compile-gate. */
    pwm_dimmer dim;
    (void) pwm_dimmer_init(&dim, GPIO_NUM_14, 1000);
    (void) pwm_dimmer_set(&dim, 50);
    ESP_LOGI(TAG, "dimmer set");
```

- [ ] **Step 11: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: NINE `... 0 Failures ... OK` lines + `strict: OK`.

ESP gate:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`. If unresolved `pwm_*`, add `pwm` to `COMPONENT_REQUIRES` and rebuild.

- [ ] **Step 12: Confirm the new symbols are in the ELF**

Run (PATH in the same command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; cd /Users/man.nk/git/clibs/esp-gate && xtensa-lx106-elf-nm build/esp_libs_gate.elf | grep -E " T (relay_set|button_poll|dht_read|pwm_dimmer_set)"
```
Expected: all four appear as `T` symbols.

- [ ] **Step 13: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/pwm_dimmer/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/pwm_dimmer): software PWM dimmer (host-tested duty map + SDK glue)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 5: Final gates + README update

**Files:**
- Modify: `esp-libs/README.md`
- Test: re-run both gates.

**Interfaces:** Consumes nothing new; documents the Task 1–4 APIs. Produces nothing.

- [ ] **Step 1: Re-run the host gate**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: NINE `... 0 Failures ... OK` lines + `strict: OK`.

- [ ] **Step 2: Re-run the ESP compile-gate**

Run:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -5 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 3: Update `esp-libs/README.md`** — two edits:

(a) Replace the Layer-3 intro bullet (currently):
```
- *Layer 3 — device drivers* (DHT, DS18B20, relay…) — later, needs hardware.
```
with:
```
- **Layer 3 — device drivers (this set):** `relay`, `button`, `dht`, `pwm_dimmer` —
  GPIO/PWM/bit-bang glue verified by the `esp-gate` link; the pure parts
  (`relay_level`, `dht_parse`, `dimmer_duty`) are host-Unity-tested. **Hardware run
  is deferred** — bit-bang timing (dht) is a first cut to tune on real hardware.
```

(b) Insert the following just before the `### Compile-gate` subsection (after the last Layer-2 `### mdns_node …` subsection):
```markdown
### `relay` — GPIO output actuator

- `relay_init(&r, GPIO_NUM_5, active_low)` / `relay_set(&r, on)` / `relay_toggle(&r)` /
  `relay_is_on(&r)`. `relay_level(on, active_low)` (pure, host-tested) maps the on-state
  to the GPIO level.

### `button` — debounced GPIO input

- `button_init(&b, pin, active_low, threshold)` then `button_poll(&b)` at a fixed
  interval → `BUTTON_PRESSED` / `BUTTON_RELEASED` / `BUTTON_NONE`. Reuses the L1
  `debounce` FSM. `button_is_pressed(&b)` for the current state.

### `dht` — DHT22 / DHT11 temperature + humidity

- `dht_parse(frame, &temp_dc, &humi_dpct)` (pure, host-tested): a 5-byte frame →
  deci-°C / deci-%RH with checksum validation.
- `dht_read(&d, &temp_dc, &humi_dpct)` bit-bangs a read (`ESP_OK` /
  `ESP_ERR_TIMEOUT` / `ESP_ERR_INVALID_CRC`). The bit-bang timing is a first cut —
  tune on hardware.

### `pwm_dimmer` — software PWM dimmer

- `pwm_dimmer_init(&d, pin, period_us)` then `pwm_dimmer_set(&d, percent)`.
  `dimmer_duty(percent, max_duty)` (pure, host-tested) maps 0–100 % to a duty.
  NOTE: the SDK PWM is a single global subsystem — one dimmer on channel 0.
```

- [ ] **Step 4: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document Layer-3 drivers relay + button + dht + pwm_dimmer

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:**
- `relay` pure level map (host-tested) + SDK glue → Task 1. ✓
- `button` over L1 debounce (compile-gate) → Task 2. ✓
- `dht` pure frame parse (host-tested) + bit-bang glue → Task 3. ✓
- `pwm_dimmer` pure duty map (host-tested) + SDK glue → Task 4. ✓
- `component.mk` adds 4 dirs to all 3 lists, `test_*.c` excluded, REQUIRES unchanged → Tasks 1–4 wiring. ✓
- `Makefile` +3 host targets → 9 suites; strict covers the 3 new pure `.c` → Tasks 1, 3, 4. ✓
- `esp-gate/main.c` touches all 4 → Tasks 1–4. ✓
- README Layer-3 section → Task 5. ✓
- Pure `.c` SDK-header-free; SDK decls in separate headers → relay/dht/pwm_dimmer header splits; button is single compile-gate file (no pure part). ✓
- Out of scope (ds18b20, OTA, node_core, hardware run) → absent. ✓

**Placeholder scan:** No TBD/TODO; every code/step complete; commands have expected output. ✓

**Type consistency:** `relay`/`relay_level` signatures match across header/impl/test/gate. `button` enum + struct + 3 funcs consistent (uses L1 `debounce_edge`/`DEBOUNCE_RISING`/`DEBOUNCE_FALLING`/`debounce_level` exactly as L1 defines). `dht_parse`/`dht` and `dimmer_duty`/`pwm_dimmer` consistent across header/impl/test/gate. `component.mk` is an incremental superset per task (relay → +button → +dht → +pwm_dimmer = final 15-dir list / 19 objs). `main.c` includes accumulate (relay.h → button.h → dht.h → pwm_dimmer.h); each touch block uses only that task's symbols. ✓

**Boundary/correctness check:** `dht_parse` checksum `(b0+b1+b2+b3)&0xFF` via `uint8_t` wraparound; sign via `b2&0x80`; verified frames (658/330, −20/658, bad-checksum→false). `dimmer_duty` uses `uint64_t` intermediate (no overflow), `>=100` clamp, `+50)/100` rounding (33→33, 50→500). `relay_level` truth table matches the test. `button_poll` maps active-low correctly (`level==0 → pressed`). `dht.c` wait loops are bounded → `ESP_ERR_TIMEOUT`. ✓
