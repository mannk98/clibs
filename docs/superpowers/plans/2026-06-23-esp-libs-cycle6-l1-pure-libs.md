# esp-libs Cycle 6 — L1 pure libs `hysteresis` + `crc` + `map_range` + `median` + `throttle` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add five small, independent, fully host-Unity-tested Layer-1 pure libs to `esp-libs` — `hysteresis`, `crc`, `map_range`, `median`, `throttle` — then wire them into the `esp_libs` component + compile-gate.

**Architecture:** Each lib is one directory of freestanding portable C (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>` only, no SDK headers) with its own Unity host test. Tasks 1–5 are **independent** (each only creates its own dir + verifies with a standalone `cc`) — they do NOT touch shared files and do NOT commit. Task 6 is the single **integration** step: it wires all five into `Makefile`, `component.mk`, and `esp-gate/main/main.c`, runs the full host gate (14 suites) + `strict` + the esp-gate link, and commits.

**Tech Stack:** portable C11, host `cc` + vendored Unity (`../common/third_party/unity`); ESP8266 cross-compile gate via `esp-gate` (formality for pure libs).

## Global Constraints

- **Tasks 1–5 scope rule:** each task creates ONLY its own `esp-libs/<lib>/` directory (`<lib>.h`, `<lib>.c`, `test_<lib>.c`) and verifies via a standalone `cc` compile+run. It MUST NOT edit `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, or any other lib; it MUST NOT `git commit`. All wiring + commits happen in Task 6.
- **Standalone verify command** (Tasks 1–5; outputs to `/tmp` to avoid the shared `build/` dir): from `/Users/man.nk/git/clibs/esp-libs`,
  `cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -I<lib> -o /tmp/test_<lib> <lib>/test_<lib>.c <lib>/<lib>.c ../common/third_party/unity/unity.c && /tmp/test_<lib>`.
- **Pure-C only:** no SDK headers; no malloc; transparent structs + `self`-methods; `CLIBS_<NAME>_H` guards; NULL/edge guards; a doc comment per public function; "Not thread-safe" where stateful.
- **Test hygiene:** real assertions on computed values (no asserts-nothing tests). Each test file has `setUp`/`tearDown` (empty) + a `main` running the cases via Unity.
- **Build env for the esp-gate (Task 6 only), as ONE command:** `export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4`.
- **Final host gate (Task 6):** `cd esp-libs && make test` → **14 suites**, all `0 Failures`; `make strict` → `strict: OK`.
- **Commits (Task 6):** end each message with the trailer `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `hysteresis` — Schmitt-trigger deadband control

**Files:** Create `esp-libs/hysteresis/hysteresis.h`, `esp-libs/hysteresis/hysteresis.c`, `esp-libs/hysteresis/test_hysteresis.c`. (No wiring, no commit — Task 6.)

**Interfaces — Produces:**
```c
typedef struct { int32_t low; int32_t high; bool on; } hysteresis;
void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial);
bool hysteresis_update(hysteresis *self, int32_t value);
bool hysteresis_state(const hysteresis *self);
```

- [ ] **Step 1: Write `esp-libs/hysteresis/test_hysteresis.c`**

```c
#include "unity.h"
#include "hysteresis.h"

void setUp(void) {}
void tearDown(void) {}

static void test_deadband_holds(void)
{
    hysteresis h;
    hysteresis_init(&h, 20, 25, false);
    TEST_ASSERT_FALSE(hysteresis_update(&h, 22));   /* in band -> hold initial false */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 26));    /* >= high -> on */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 24));    /* in band -> hold on */
    TEST_ASSERT_FALSE(hysteresis_update(&h, 20));   /* <= low -> off */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 25));    /* >= high -> on */
}

static void test_boundary_inclusive(void)
{
    hysteresis h;
    hysteresis_init(&h, 10, 30, false);
    TEST_ASSERT_TRUE(hysteresis_update(&h, 30));    /* == high inclusive */
    TEST_ASSERT_FALSE(hysteresis_update(&h, 10));   /* == low inclusive */
}

static void test_swap(void)
{
    hysteresis h;
    hysteresis_init(&h, 25, 20, false);             /* low>high -> swapped */
    TEST_ASSERT_TRUE(hysteresis_update(&h, 25));
    TEST_ASSERT_FALSE(hysteresis_update(&h, 20));
}

static void test_null(void)
{
    TEST_ASSERT_FALSE(hysteresis_update(NULL, 5));
    TEST_ASSERT_FALSE(hysteresis_state(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_deadband_holds);
    RUN_TEST(test_boundary_inclusive);
    RUN_TEST(test_swap);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/hysteresis/hysteresis.h`**

```c
#ifndef CLIBS_HYSTERESIS_H
#define CLIBS_HYSTERESIS_H

#include <stdint.h>
#include <stdbool.h>

/* Rising-edge Schmitt trigger: output goes ON at value >= high, OFF at value <= low,
 * and holds in the (low, high) deadband. Not thread-safe. */
typedef struct {
    int32_t low;
    int32_t high;
    bool    on;
} hysteresis;

/* Init thresholds + initial output. If low > high, they are swapped. */
void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial);
/* Feed a value; returns the (possibly updated) output. */
bool hysteresis_update(hysteresis *self, int32_t value);
/* Current output (false if self is NULL). */
bool hysteresis_state(const hysteresis *self);

#endif /* CLIBS_HYSTERESIS_H */
```

- [ ] **Step 3: Verify the test FAILS** (run the Global-Constraints standalone command with `<lib>=hysteresis`). Expected: `hysteresis.c: No such file` / undefined symbols.

- [ ] **Step 4: Write `esp-libs/hysteresis/hysteresis.c`**

```c
#include "hysteresis.h"

void hysteresis_init(hysteresis *self, int32_t low, int32_t high, bool initial)
{
    if (self == NULL) {
        return;
    }
    if (low > high) {
        int32_t tmp = low;
        low = high;
        high = tmp;
    }
    self->low = low;
    self->high = high;
    self->on = initial;
}

bool hysteresis_update(hysteresis *self, int32_t value)
{
    if (self == NULL) {
        return false;
    }
    if (value >= self->high) {
        self->on = true;
    } else if (value <= self->low) {
        self->on = false;
    }
    return self->on;
}

bool hysteresis_state(const hysteresis *self)
{
    return (self != NULL) && self->on;
}
```

- [ ] **Step 5: Verify the test PASSES** (rerun the standalone command). Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

---

### Task 2: `crc` — CRC-8/MAXIM + CRC-16/MODBUS

**Files:** Create `esp-libs/crc/crc.h`, `esp-libs/crc/crc.c`, `esp-libs/crc/test_crc.c`. (No wiring, no commit — Task 6.)

**Interfaces — Produces:**
```c
uint8_t  crc8_maxim(const uint8_t *data, size_t len);
uint16_t crc16_modbus(const uint8_t *data, size_t len);
```

- [ ] **Step 1: Write `esp-libs/crc/test_crc.c`**

```c
#include "unity.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

/* CRC catalogue "check" input: ASCII "123456789". */
static const uint8_t CHK[9] = {'1','2','3','4','5','6','7','8','9'};

static void test_crc8_maxim_check(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xA1, crc8_maxim(CHK, 9));
}

static void test_crc16_modbus_check(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x4B37, crc16_modbus(CHK, 9));
}

static void test_empty(void)
{
    uint8_t d = 0;
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(&d, 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus(&d, 0));
}

static void test_null(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(NULL, 5));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus(NULL, 5));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc8_maxim_check);
    RUN_TEST(test_crc16_modbus_check);
    RUN_TEST(test_empty);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/crc/crc.h`**

```c
#ifndef CLIBS_CRC_H
#define CLIBS_CRC_H

#include <stdint.h>
#include <stddef.h>

/* CRC-8/MAXIM (Dallas/Maxim 1-Wire): reflected poly 0x8C, init 0x00. For DS18B20 etc.
 * NULL data -> returns the init value 0x00. */
uint8_t crc8_maxim(const uint8_t *data, size_t len);
/* CRC-16/MODBUS: reflected poly 0xA001, init 0xFFFF. NULL data -> returns 0xFFFF. */
uint16_t crc16_modbus(const uint8_t *data, size_t len);

#endif /* CLIBS_CRC_H */
```

- [ ] **Step 3: Verify the test FAILS** (standalone command, `<lib>=crc`). Expected: `crc.c: No such file` / undefined symbols.

- [ ] **Step 4: Write `esp-libs/crc/crc.c`**

```c
#include "crc.h"

uint8_t crc8_maxim(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    if (data == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint8_t) ((crc >> 1) ^ 0x8C) : (uint8_t) (crc >> 1);
        }
    }
    return crc;
}

uint16_t crc16_modbus(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    if (data == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint16_t) ((crc >> 1) ^ 0xA001) : (uint16_t) (crc >> 1);
        }
    }
    return crc;
}
```

- [ ] **Step 5: Verify the test PASSES** (rerun). Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

---

### Task 3: `map_range` — integer map + clamp

**Files:** Create `esp-libs/map_range/map_range.h`, `esp-libs/map_range/map_range.c`, `esp-libs/map_range/test_map_range.c`. (No wiring, no commit — Task 6.)

**Interfaces — Produces:**
```c
int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi);
```

- [ ] **Step 1: Write `esp-libs/map_range/test_map_range.c`**

```c
#include "unity.h"
#include "map_range.h"

void setUp(void) {}
void tearDown(void) {}

static void test_map_basic(void)
{
    TEST_ASSERT_EQUAL_INT32(50,  map_range(512, 0, 1023, 0, 100));
    TEST_ASSERT_EQUAL_INT32(0,   map_range(0, 0, 1023, 0, 100));
    TEST_ASSERT_EQUAL_INT32(100, map_range(1023, 0, 1023, 0, 100));
}

static void test_map_inverted(void)
{
    TEST_ASSERT_EQUAL_INT32(50,  map_range(5, 0, 10, 100, 0));
    TEST_ASSERT_EQUAL_INT32(100, map_range(0, 0, 10, 100, 0));
}

static void test_map_degenerate(void)
{
    TEST_ASSERT_EQUAL_INT32(42, map_range(7, 5, 5, 42, 99));   /* in_min==in_max -> out_min */
}

static void test_clamp(void)
{
    TEST_ASSERT_EQUAL_INT32(100, clamp_i32(150, 0, 100));
    TEST_ASSERT_EQUAL_INT32(0,   clamp_i32(-5, 0, 100));
    TEST_ASSERT_EQUAL_INT32(50,  clamp_i32(50, 0, 100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_map_basic);
    RUN_TEST(test_map_inverted);
    RUN_TEST(test_map_degenerate);
    RUN_TEST(test_clamp);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/map_range/map_range.h`**

```c
#ifndef CLIBS_MAP_RANGE_H
#define CLIBS_MAP_RANGE_H

#include <stdint.h>

/* Linear map of x from [in_min,in_max] to [out_min,out_max] (no clamping; int64
 * intermediate so it is overflow-safe for int32 inputs). in_min==in_max -> out_min. */
int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
/* Clamp x to [lo,hi] (assumes lo <= hi). */
int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi);

#endif /* CLIBS_MAP_RANGE_H */
```

- [ ] **Step 3: Verify the test FAILS** (standalone command, `<lib>=map_range`). Expected: undefined symbols.

- [ ] **Step 4: Write `esp-libs/map_range/map_range.c`**

```c
#include "map_range.h"

int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    if (in_min == in_max) {
        return out_min;
    }
    int64_t num = (int64_t) (x - in_min) * (int64_t) (out_max - out_min);
    return (int32_t) (num / (int64_t) (in_max - in_min) + out_min);
}

int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}
```

- [ ] **Step 5: Verify the test PASSES** (rerun). Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

---

### Task 4: `median` — median-of-3 despike

**Files:** Create `esp-libs/median/median.h`, `esp-libs/median/median.c`, `esp-libs/median/test_median.c`. (No wiring, no commit — Task 6.)

**Interfaces — Produces:**
```c
int32_t median3(int32_t a, int32_t b, int32_t c);
```

- [ ] **Step 1: Write `esp-libs/median/test_median.c`**

```c
#include "unity.h"
#include "median.h"

void setUp(void) {}
void tearDown(void) {}

static void test_median3(void)
{
    TEST_ASSERT_EQUAL_INT32(2,  median3(1, 2, 3));
    TEST_ASSERT_EQUAL_INT32(2,  median3(3, 1, 2));
    TEST_ASSERT_EQUAL_INT32(5,  median3(5, 5, 1));
    TEST_ASSERT_EQUAL_INT32(-2, median3(-3, -1, -2));
    TEST_ASSERT_EQUAL_INT32(7,  median3(7, 7, 7));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_median3);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/median/median.h`**

```c
#ifndef CLIBS_MEDIAN_H
#define CLIBS_MEDIAN_H

#include <stdint.h>

/* Median of three values (despike). Apply over the last 3 samples of a stream. */
int32_t median3(int32_t a, int32_t b, int32_t c);

#endif /* CLIBS_MEDIAN_H */
```

- [ ] **Step 3: Verify the test FAILS** (standalone command, `<lib>=median`). Expected: undefined symbols.

- [ ] **Step 4: Write `esp-libs/median/median.c`**

```c
#include "median.h"

int32_t median3(int32_t a, int32_t b, int32_t c)
{
    int32_t tmp;
    if (a > b) { tmp = a; a = b; b = tmp; }   /* a <= b */
    if (b > c) { tmp = b; b = c; c = tmp; }   /* b <= c */
    if (a > b) { tmp = a; a = b; b = tmp; }   /* a <= b */
    return b;                                  /* the middle value */
}
```

- [ ] **Step 5: Verify the test PASSES** (rerun). Expected: `1 Tests 0 Failures 0 Ignored` / `OK`.

---

### Task 5: `throttle` — minimum interval gate

**Files:** Create `esp-libs/throttle/throttle.h`, `esp-libs/throttle/throttle.c`, `esp-libs/throttle/test_throttle.c`. (No wiring, no commit — Task 6.)

**Interfaces — Produces:**
```c
typedef struct { uint32_t interval_ms; uint32_t last_ms; bool primed; } throttle;
void throttle_init(throttle *self, uint32_t interval_ms);
bool throttle_allow(throttle *self, uint32_t now_ms);
```

- [ ] **Step 1: Write `esp-libs/throttle/test_throttle.c`**

```c
#include "unity.h"
#include "throttle.h"

void setUp(void) {}
void tearDown(void) {}

static void test_basic(void)
{
    throttle t;
    throttle_init(&t, 1000);
    TEST_ASSERT_TRUE(throttle_allow(&t, 0));        /* first call -> true */
    TEST_ASSERT_FALSE(throttle_allow(&t, 500));     /* < interval */
    TEST_ASSERT_TRUE(throttle_allow(&t, 1000));     /* == interval */
    TEST_ASSERT_FALSE(throttle_allow(&t, 1500));
    TEST_ASSERT_TRUE(throttle_allow(&t, 2000));
}

static void test_wraparound(void)
{
    throttle t;
    throttle_init(&t, 10);
    TEST_ASSERT_TRUE(throttle_allow(&t, 0xFFFFFFF0u));   /* prime near wrap */
    TEST_ASSERT_TRUE(throttle_allow(&t, 0x00000005u));   /* elapsed 21 across the wrap >= 10 */
}

static void test_null(void)
{
    TEST_ASSERT_FALSE(throttle_allow(NULL, 100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/throttle/throttle.h`**

```c
#ifndef CLIBS_THROTTLE_H
#define CLIBS_THROTTLE_H

#include <stdint.h>
#include <stdbool.h>

/* Minimum-interval gate (e.g. rate-limit telemetry publishes). The caller supplies
 * the current time in ms (from millis / esp_timer / the periodic helper). Not thread-safe. */
typedef struct {
    uint32_t interval_ms;
    uint32_t last_ms;
    bool     primed;
} throttle;

void throttle_init(throttle *self, uint32_t interval_ms);
/* First call -> true (primes). Otherwise true (and updates) once interval_ms has
 * elapsed since the last allow; false meanwhile. Wrap-safe unsigned subtraction. */
bool throttle_allow(throttle *self, uint32_t now_ms);

#endif /* CLIBS_THROTTLE_H */
```

- [ ] **Step 3: Verify the test FAILS** (standalone command, `<lib>=throttle`). Expected: undefined symbols.

- [ ] **Step 4: Write `esp-libs/throttle/throttle.c`**

```c
#include "throttle.h"

void throttle_init(throttle *self, uint32_t interval_ms)
{
    if (self == NULL) {
        return;
    }
    self->interval_ms = interval_ms;
    self->last_ms = 0;
    self->primed = false;
}

bool throttle_allow(throttle *self, uint32_t now_ms)
{
    if (self == NULL) {
        return false;
    }
    if (!self->primed) {
        self->primed = true;
        self->last_ms = now_ms;
        return true;
    }
    if ((uint32_t) (now_ms - self->last_ms) >= self->interval_ms) {
        self->last_ms = now_ms;
        return true;
    }
    return false;
}
```

- [ ] **Step 5: Verify the test PASSES** (rerun). Expected: `3 Tests 0 Failures 0 Ignored` / `OK`.

---

### Task 6: Integration — wire all 5 into Makefile + component.mk + esp-gate, run gates, commit

**Files:** Modify `esp-libs/Makefile`, `esp-libs/component.mk`, `esp-gate/main/main.c`, `esp-libs/README.md`. (Depends on Tasks 1–5 being present on disk.)

**Interfaces — Consumes:** the five libs' public symbols (see each task's Produces block).

- [ ] **Step 1: Add the 5 host-test targets to `esp-libs/Makefile`** — three edits:

(a) Replace the `.PHONY` and `test:` lines with:
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle
```
(b) Add these five target blocks after the `dimmer_duty:` target block:
```makefile
hysteresis: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihysteresis -o $(BUILD)/test_hysteresis \
	    hysteresis/test_hysteresis.c hysteresis/hysteresis.c $(UNITY)/unity.c
	./$(BUILD)/test_hysteresis

crc: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Icrc -o $(BUILD)/test_crc \
	    crc/test_crc.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_crc

map_range: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imap_range -o $(BUILD)/test_map_range \
	    map_range/test_map_range.c map_range/map_range.c $(UNITY)/unity.c
	./$(BUILD)/test_map_range

median: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imedian -o $(BUILD)/test_median \
	    median/test_median.c median/median.c $(UNITY)/unity.c
	./$(BUILD)/test_median

throttle: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ithrottle -o $(BUILD)/test_throttle \
	    throttle/test_throttle.c throttle/throttle.c $(UNITY)/unity.c
	./$(BUILD)/test_throttle
```
(c) Add these five lines to the `strict:` recipe (before `@echo "strict: OK"`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihysteresis -c hysteresis/hysteresis.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Icrc        -c crc/crc.c               -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imap_range  -c map_range/map_range.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imedian     -c median/median.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ithrottle   -c throttle/throttle.c     -o /dev/null
```

- [ ] **Step 2: Wire the 5 dirs into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle
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
                  median/median.o throttle/throttle.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 3: Touch the 5 symbols in `esp-gate/main/main.c`** — add after `#include "pwm_dimmer.h"`:

```c
#include "hysteresis.h"
#include "crc.h"
#include "map_range.h"
#include "median.h"
#include "throttle.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* L1 extras compile-gate. */
    hysteresis hy;
    hysteresis_init(&hy, 20, 25, false);
    (void) hysteresis_update(&hy, 22);
    uint8_t crcbuf[3] = {1, 2, 3};
    (void) crc8_maxim(crcbuf, 3);
    (void) crc16_modbus(crcbuf, 3);
    (void) map_range(512, 0, 1023, 0, 100);
    (void) clamp_i32(150, 0, 100);
    (void) median3(1, 2, 3);
    throttle th;
    throttle_init(&th, 1000);
    (void) throttle_allow(&th, 0);
    ESP_LOGI(TAG, "l1 extras linked");
```

- [ ] **Step 4: Run the host gate**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: **14** `... 0 Failures ... OK` lines + `strict: OK`.

- [ ] **Step 5: Run the esp-gate compile-gate**

Run (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 6: Commit the libs + wiring**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/hysteresis/ esp-libs/crc/ esp-libs/map_range/ esp-libs/median/ esp-libs/throttle/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs): L1 pure libs hysteresis + crc + map_range + median + throttle

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

- [ ] **Step 7: Update `esp-libs/README.md` — extend the Layer-1 table**

In the `## Layer-1 libraries` table, add these rows after the `mqtt_topic` row:
```
| `hysteresis` | Schmitt-trigger deadband on/off control (`hysteresis_init/update/state`). |
| `crc` | `crc8_maxim` (1-Wire) + `crc16_modbus` checksums. |
| `map_range` | Integer linear map + `clamp_i32` (ADC counts → real units). |
| `median` | `median3` despike (median of three). |
| `throttle` | Minimum-interval gate (`throttle_init/allow`) for rate-limiting. |
```

- [ ] **Step 8: Commit the docs**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document L1 hysteresis + crc + map_range + median + throttle

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:** hysteresis (Task 1), crc (Task 2), map_range (Task 3), median/median3 (Task 4), throttle (Task 5); component.mk + Makefile (14 suites) + esp-gate wiring + README (Task 6). All five spec modules + infra covered. ✓

**Placeholder scan:** No TBD/TODO; every lib has complete header+impl+test code; commands have expected output. ✓

**Type consistency:** Each lib's signatures match across its header/impl/test and the Task 6 esp-gate touch (`hysteresis_init/update`, `crc8_maxim`/`crc16_modbus`, `map_range`/`clamp_i32`, `median3`, `throttle_init/allow`). `component.mk` OBJS lists exactly one `.o` per lib (5 new) appended to the cycle-5 list; Makefile `.PHONY`/`test:` list the 5 new suites; both pure `.c` of each lib covered by `strict`. ✓

**Test-vector check:** crc check values are the CRC catalogue standards (CRC-8/MAXIM "123456789"=0xA1; CRC-16/MODBUS "123456789"=0x4B37). `map_range(512,0,1023,0,100)`=51200/1023=50; inverted `(5,0,10,100,0)`=−500/10+100=50. `median3` swap network returns the middle. `throttle` wrap: `0x05−0xFFFFFFF0` (mod 2³²) = 21 ≥ 10 → true. `hysteresis` inclusive bounds (`>=high`, `<=low`) + deadband hold. ✓
