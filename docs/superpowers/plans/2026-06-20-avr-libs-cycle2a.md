# avr-libs Cycle 2a (Pure-Logic Modules) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the four pure-logic avr-libs modules (`myPid`, `goot80_typeK`, `t33Soldering`, `ringBuffer`) host-unit-tested with Unity, bug-fixed, and AVR-cross-compile-clean — without touching the register drivers.

**Architecture:** Each module is host-compilable C tested with vendored Unity (`make test`), verified to still build for atmega328p via an `avr-gcc` gate (`make avr`), and warning-clean under `-Werror` (`make strict`). `ringBuffer` is rewritten to a malloc-free byte FIFO over caller storage; the two soldering modules are de-collided and made host-buildable; `myPid` gets characterization tests.

**Tech Stack:** C11/C99, Unity (ThrowTheSwitch, vendored), GNU make, host `cc`, `avr-gcc` 9.5 (keg-only at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc`).

**Spec:** `clibs/docs/superpowers/specs/2026-06-20-avr-libs-cycle2a-pure-logic-design.md`

---

## IMPORTANT — where this runs

- **All code lives in the `avr-libs` git submodule**, on disk at `/Users/man.nk/git/clibs/avr-libs/`. It is its OWN git repo (`mannk98/avr-libs`). Every `git` command below runs **inside `/Users/man.nk/git/clibs/avr-libs`** and commits to that repo.
- The controller creates a feature branch in `avr-libs` BEFORE Task 1 and, AFTER Task 6, merges it there, pushes `avr-libs`, then bumps the gitlink in the parent `clibs` repo. Implementer subagents must **NOT** switch branches.
- Append this trailer to every commit message:
  ```
  Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
  ```

## File Structure

| File | Responsibility |
|------|----------------|
| `third_party/unity/unity.{c,h}`, `unity_internals.h`, `LICENSE.txt` | Vendored Unity |
| `Makefile` | `make test` (4 host suites), `make avr` (cross-compile gate), `make strict`, `make clean` |
| `.gitignore` | ignore `build/` |
| `myPid/PID.{c,h}` | (existing) PID controller — unchanged logic |
| `myPid/test_pid.c` | Unity characterization tests for PID |
| `goot80_typeK/typeK.{c,h}` | refactored: `goot80_vol_to_temp`, stdint, dead code removed |
| `goot80_typeK/test_goot80.c` | Unity tests |
| `t33Soldering/T33Soldering.{c,h}` | refactored: `t33_vol_to_temp`, stdint, dead code removed |
| `t33Soldering/test_t33.c` | Unity tests |
| `ringBuffer/ringbuff.{c,h}` | rewritten malloc-free byte FIFO |
| `ringBuffer/test_ringbuf.c` | Unity tests |

(The existing `ringBuffer/test.c` — an ad-hoc `main()` — is removed in Task 5.)

---

## Task 1: Scaffold — Unity, Makefile, .gitignore (in avr-libs)

**Files:** Create `third_party/unity/{unity.c,unity.h,unity_internals.h,LICENSE.txt}`, `Makefile`, `.gitignore`.

- [ ] **Step 1: Create dirs**
```bash
cd /Users/man.nk/git/clibs/avr-libs
mkdir -p third_party/unity build
```

- [ ] **Step 2: Vendor Unity (4 files) from the pinned release**
```bash
cd /Users/man.nk/git/clibs/avr-libs/third_party/unity
BASE=https://raw.githubusercontent.com/ThrowTheSwitch/Unity/v2.6.0
curl -fsSL $BASE/src/unity.c -o unity.c
curl -fsSL $BASE/src/unity.h -o unity.h
curl -fsSL $BASE/src/unity_internals.h -o unity_internals.h
curl -fsSL $BASE/LICENSE.txt -o LICENSE.txt
ls -1   # expect: LICENSE.txt unity.c unity.h unity_internals.h
```
If a URL 404s, replace `v2.6.0` with `master`.

- [ ] **Step 3: Write `Makefile`** (recipe lines MUST use TAB indentation):
```makefile
# Host build + test runner + AVR cross-compile gate for avr-libs pure-logic modules.
CC       ?= cc
AVR_CC   ?= /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc
CFLAGS   := -std=c11 -Wall -Wextra -g
AVRFLAGS := -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra
UNITY    := third_party/unity
BUILD    := build

.PHONY: test pid goot80 t33 ringbuf strict avr clean
test: pid goot80 t33 ringbuf

$(BUILD):
	mkdir -p $(BUILD)

pid: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -ImyPid -o $(BUILD)/test_pid \
	    myPid/test_pid.c myPid/PID.c $(UNITY)/unity.c
	./$(BUILD)/test_pid

goot80: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Igoot80_typeK -o $(BUILD)/test_goot80 \
	    goot80_typeK/test_goot80.c goot80_typeK/typeK.c $(UNITY)/unity.c
	./$(BUILD)/test_goot80

t33: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -It33Soldering -o $(BUILD)/test_t33 \
	    t33Soldering/test_t33.c t33Soldering/T33Soldering.c $(UNITY)/unity.c
	./$(BUILD)/test_t33

ringbuf: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -IringBuffer -o $(BUILD)/test_ringbuf \
	    ringBuffer/test_ringbuf.c ringBuffer/ringbuff.c $(UNITY)/unity.c
	./$(BUILD)/test_ringbuf

# Host warning-clean gate for the owned modules.
strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -ImyPid        -c myPid/PID.c                  -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Igoot80_typeK -c goot80_typeK/typeK.c          -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -It33Soldering -c t33Soldering/T33Soldering.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -IringBuffer   -c ringBuffer/ringbuff.c         -o /dev/null
	@echo "strict: OK"

# AVR cross-compile gate: confirms the modules still build for atmega328p.
avr:
	$(AVR_CC) $(AVRFLAGS) -c myPid/PID.c                -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c goot80_typeK/typeK.c       -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c t33Soldering/T33Soldering.c -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c ringBuffer/ringbuff.c      -o /dev/null
	@echo "avr: OK"

clean:
	rm -rf $(BUILD)
```

- [ ] **Step 4: Write `.gitignore`**
```
build/
```

- [ ] **Step 5: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add Makefile .gitignore third_party/unity
git commit -m "build(avr-libs): vendor Unity, add host test + avr-gcc compile-gate Makefile"
```
(Append the Co-Authored-By trailer.)

Note: `make test`/`strict`/`avr` will not fully succeed yet (test files + refactors come next). Optionally sanity-check Unity compiles: `cc -c -Ithird_party/unity third_party/unity/unity.c -o /tmp/u.o`.

---

## Task 2: `myPid` — characterization tests (existing logic unchanged)

`myPid/PID.{c,h}` is already pure float math and host-compilable. We ADD tests only; no implementation change. These are characterization tests, so they pass immediately (no RED phase).

**Files:** Create `myPid/test_pid.c`.

- [ ] **Step 1: Write the test**

Create `myPid/test_pid.c`:
```c
#include "unity.h"
#include "PID.h"

void setUp(void) {}
void tearDown(void) {}

/* helper: a PID with wide limits unless a test overrides */
static void base_pid(PIDController *p) {
    p->Kp = 0; p->Ki = 0; p->Kd = 0;
    p->tau = 0.02f; p->T = 0.1f;
    p->limMin = -1e6f; p->limMax = 1e6f;
    p->limMinInt = -1e6f; p->limMaxInt = 1e6f;
    PIDController_Init(p);
}

static void test_proportional_only(void) {
    PIDController p; base_pid(&p);
    p.Kp = 2.0f;
    float out = PIDController_Update(&p, 10.0f, 0.0f); /* error = 10 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 20.0f, out);      /* Kp*error */
}

static void test_output_clamp(void) {
    PIDController p; base_pid(&p);
    p.Kp = 2.0f; p.limMax = 5.0f;
    float out = PIDController_Update(&p, 10.0f, 0.0f); /* raw 20 -> clamp 5 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 5.0f, out);
}

static void test_integrator_antiwindup(void) {
    PIDController p; base_pid(&p);
    p.Ki = 2.0f; p.T = 1.0f; p.limMaxInt = 3.0f;
    /* integrator = 0 + 0.5*Ki*T*(error+prevError) = 0.5*2*1*(10+0)=10 -> clamp 3 */
    float out = PIDController_Update(&p, 10.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 3.0f, out);
}

static void test_derivative_on_measurement_sign(void) {
    PIDController p; base_pid(&p);
    p.Kd = 1.0f;
    PIDController_Update(&p, 0.0f, 0.0f);             /* prime prevMeasurement=0 */
    float out = PIDController_Update(&p, 0.0f, 5.0f); /* measurement rose 0->5 */
    TEST_ASSERT_TRUE(out < 0.0f);                     /* derivative opposes rise */
}

static void test_p_only_converges_to_setpoint(void) {
    /* P controller on a pure-integrator plant (meas += T*out) converges to sp */
    PIDController p; base_pid(&p);
    p.Kp = 0.5f; p.T = 0.1f;
    float meas = 0.0f, sp = 100.0f;
    for (int i = 0; i < 500; i++) {
        float out = PIDController_Update(&p, sp, meas);
        meas += p.T * out;
    }
    TEST_ASSERT_FLOAT_WITHIN(0.5f, sp, meas);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_proportional_only);
    RUN_TEST(test_output_clamp);
    RUN_TEST(test_integrator_antiwindup);
    RUN_TEST(test_derivative_on_measurement_sign);
    RUN_TEST(test_p_only_converges_to_setpoint);
    return UNITY_END();
}
```

- [ ] **Step 2: Run the test (expect PASS — characterizing existing code)**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make pid`
Expected: `5 Tests 0 Failures 0 Ignored` / `OK`. If any FAIL, the test's expected value is wrong (not the impl) — re-derive from `PID.c` and fix the test; do NOT change `PID.c`.

- [ ] **Step 3: Confirm host strict + AVR compile of PID**

Run:
```bash
cd /Users/man.nk/git/clibs/avr-libs
cc -std=c11 -Wall -Wextra -Werror -ImyPid -c myPid/PID.c -o /dev/null && echo HOST_OK
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c myPid/PID.c -o /dev/null && echo AVR_OK
```
Expected: `HOST_OK` and `AVR_OK`.

- [ ] **Step 4: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add myPid/test_pid.c
git commit -m "test(avr-libs/myPid): add Unity characterization tests for PID"
```

---

## Task 3: `goot80_typeK` — refactor + tests

Refactor `typeK.{c,h}`: drop `<avr/io.h>` (use `<stdint.h>`), remove dead commented code, rename `volToTemp` → `goot80_vol_to_temp`, guard → `CLIBS_GOOT80_TYPEK_H`. Preserve the curve formula exactly.

**Files:** Modify `goot80_typeK/typeK.h`, `goot80_typeK/typeK.c`. Create `goot80_typeK/test_goot80.c`.

- [ ] **Step 1: Write the failing test**

Create `goot80_typeK/test_goot80.c`:
```c
#include "unity.h"
#include "typeK.h"

void setUp(void) {}
void tearDown(void) {}

/* Expected values are re-derived from the documented curve so the test pins the
 * formula independently of the implementation. v = adc * 0.02237 (mV). */
static void test_adc_zero_is_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, goot80_vol_to_temp(0));
}
static void test_band1(void) {            /* adc=100 -> v=2.237 (<=7.739) */
    float v = 100 * 0.02237;
    float expected = 24.95 * v - 3.12;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, goot80_vol_to_temp(100));
}
static void test_band2(void) {            /* adc=400 -> v=8.948 (<=10.971) */
    float v = 400 * 0.02237;
    float expected = 24.7 * v - 0.97;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, goot80_vol_to_temp(400));
}
static void test_band3(void) {            /* adc=550 -> v=12.3035 (<=14.293) */
    float v = 550 * 0.02237;
    float expected = 24.1 * v + 9.67;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, goot80_vol_to_temp(550));
}
static void test_band4(void) {            /* adc=700 -> v=15.659 (>14.293) */
    float v = 700 * 0.02237;
    float expected = 23.8 * v + 44.68;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, goot80_vol_to_temp(700));
}
static void test_monotonic_increasing(void) {
    TEST_ASSERT_TRUE(goot80_vol_to_temp(700) > goot80_vol_to_temp(100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_adc_zero_is_zero);
    RUN_TEST(test_band1);
    RUN_TEST(test_band2);
    RUN_TEST(test_band3);
    RUN_TEST(test_band4);
    RUN_TEST(test_monotonic_increasing);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make goot80`
Expected: FAIL — host compile error (`typeK.h` still includes `<avr/io.h>`) and/or undefined `goot80_vol_to_temp` (still named `volToTemp`).

- [ ] **Step 3: Refactor the header**

Replace the entire contents of `goot80_typeK/typeK.h` with:
```c
/*
 * typeK.h — GOOT-80 style soldering-iron thermocouple ADC->degC curve.
 */
#ifndef CLIBS_GOOT80_TYPEK_H
#define CLIBS_GOOT80_TYPEK_H

#include <stdint.h>

/* Convert a raw 10-bit ADC reading to temperature (degC) for the GOOT-80 iron.
 * Returns 0 for adc_value == 0 (treated as not-connected). */
float goot80_vol_to_temp(uint16_t adc_value);

#endif /* CLIBS_GOOT80_TYPEK_H */
```

- [ ] **Step 4: Refactor the implementation**

Replace the entire contents of `goot80_typeK/typeK.c` with (formula preserved byte-for-byte; offsets were 0):
```c
/*
 * typeK.c — GOOT-80 style soldering-iron thermocouple ADC->degC curve.
 */
#include "typeK.h"

/* thermocouple voltage breakpoints (mV) */
#define VOLTAGE_AT_190 7.739
#define VOLTAGE_AT_270 10.971
#define VOLTAGE_AT_350 14.293

float goot80_vol_to_temp(uint16_t adc_value) {
    float realTemp = 0;
    float thermoVoltage = adc_value * 0.02237; /* mV */
    if (thermoVoltage == 0)
        return 0; /* not connected */

    if (thermoVoltage > 0 && thermoVoltage <= VOLTAGE_AT_190)
        realTemp = 24.95 * thermoVoltage - 3.12;
    else if (thermoVoltage > VOLTAGE_AT_190 && thermoVoltage <= VOLTAGE_AT_270)
        realTemp = 24.7 * thermoVoltage - 0.97;
    else if (thermoVoltage > VOLTAGE_AT_270 && thermoVoltage <= VOLTAGE_AT_350)
        realTemp = 24.1 * thermoVoltage + 9.67;
    else if (thermoVoltage > VOLTAGE_AT_350)
        realTemp = 23.8 * thermoVoltage + 44.68;

    return realTemp;
}
```

- [ ] **Step 5: Run to verify it passes**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make goot80`
Expected: `6 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Confirm host strict + AVR compile**
```bash
cd /Users/man.nk/git/clibs/avr-libs
cc -std=c11 -Wall -Wextra -Werror -Igoot80_typeK -c goot80_typeK/typeK.c -o /dev/null && echo HOST_OK
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c goot80_typeK/typeK.c -o /dev/null && echo AVR_OK
```
Expected: `HOST_OK` and `AVR_OK`.

- [ ] **Step 7: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add goot80_typeK
git commit -m "refactor(avr-libs/goot80_typeK): host-buildable, rename to goot80_vol_to_temp, add tests

Drop <avr/io.h>, remove dead code, unique symbol+guard (de-collide from t33)."
```

---

## Task 4: `t33Soldering` — refactor + tests

Same treatment: drop `<avr/io.h>`, remove dead code, rename `volToTemp` → `t33_vol_to_temp`, guard → `CLIBS_T33_SOLDERING_H`. Preserve formula (offsets 25/32/23/19).

**Files:** Modify `t33Soldering/T33Soldering.h`, `t33Soldering/T33Soldering.c`. Create `t33Soldering/test_t33.c`.

- [ ] **Step 1: Write the failing test**

Create `t33Soldering/test_t33.c`:
```c
#include "unity.h"
#include "T33Soldering.h"

void setUp(void) {}
void tearDown(void) {}

/* v = adc * 0.00716 (mV); offsets 25/32/23/19 per band. */
static void test_adc_zero_is_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, t33_vol_to_temp(0));
}
static void test_band1(void) {            /* adc=300 -> v=2.148 (<=2.919) */
    float v = 300 * 0.00716;
    float expected = 62.98 * v + 7.74 + 25;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, t33_vol_to_temp(300));
}
static void test_band2(void) {            /* adc=500 -> v=3.58 (<=4.321) */
    float v = 500 * 0.00716;
    float expected = 57.07 * v + 23.74 + 32;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, t33_vol_to_temp(500));
}
static void test_band3(void) {            /* adc=700 -> v=5.012 (<=5.789) */
    float v = 700 * 0.00716;
    float expected = 54.47 * v + 34.88 + 23;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, t33_vol_to_temp(700));
}
static void test_band4(void) {            /* adc=900 -> v=6.444 (>5.789) */
    float v = 900 * 0.00716;
    float expected = 52.77 * v + 44.68 + 19;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)expected, t33_vol_to_temp(900));
}
static void test_monotonic_increasing(void) {
    TEST_ASSERT_TRUE(t33_vol_to_temp(900) > t33_vol_to_temp(300));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_adc_zero_is_zero);
    RUN_TEST(test_band1);
    RUN_TEST(test_band2);
    RUN_TEST(test_band3);
    RUN_TEST(test_band4);
    RUN_TEST(test_monotonic_increasing);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make t33`
Expected: FAIL — `<avr/io.h>` host error and/or undefined `t33_vol_to_temp`.

- [ ] **Step 3: Refactor the header**

Replace the entire contents of `t33Soldering/T33Soldering.h` with:
```c
/*
 * T33Soldering.h — T33/type-C style soldering-iron thermocouple ADC->degC curve.
 */
#ifndef CLIBS_T33_SOLDERING_H
#define CLIBS_T33_SOLDERING_H

#include <stdint.h>

/* Convert a raw 10-bit ADC reading to temperature (degC) for the T33 iron.
 * Returns 0 for adc_value == 0 (treated as not-connected). */
float t33_vol_to_temp(uint16_t adc_value);

#endif /* CLIBS_T33_SOLDERING_H */
```

- [ ] **Step 4: Refactor the implementation**

Replace the entire contents of `t33Soldering/T33Soldering.c` with (formula + offsets preserved byte-for-byte):
```c
/*
 * T33Soldering.c — T33/type-C style soldering-iron thermocouple ADC->degC curve.
 */
#include "T33Soldering.h"

/* thermocouple voltage breakpoints (mV) */
#define VOLTAGE_AT_190 2.919
#define VOLTAGE_AT_270 4.321
#define VOLTAGE_AT_350 5.789

/* per-band offsets from the op-amp configuration */
#define OFFSET_under190 25
#define OFFSET_190to270 32
#define OFFSET_270to350 23
#define OFFSET_over350  19

float t33_vol_to_temp(uint16_t adc_value) {
    float realTemp = 0;
    float thermoVoltage = adc_value * 0.00716; /* mV */
    if (thermoVoltage == 0)
        return 0; /* not connected */

    if (thermoVoltage > 0 && thermoVoltage <= VOLTAGE_AT_190)
        realTemp = 62.98 * thermoVoltage + 7.74 + OFFSET_under190;
    else if (thermoVoltage > VOLTAGE_AT_190 && thermoVoltage <= VOLTAGE_AT_270)
        realTemp = 57.07 * thermoVoltage + 23.74 + OFFSET_190to270;
    else if (thermoVoltage > VOLTAGE_AT_270 && thermoVoltage <= VOLTAGE_AT_350)
        realTemp = 54.47 * thermoVoltage + 34.88 + OFFSET_270to350;
    else if (thermoVoltage > VOLTAGE_AT_350)
        realTemp = 52.77 * thermoVoltage + 44.68 + OFFSET_over350;

    return realTemp;
}
```

- [ ] **Step 5: Run to verify it passes**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make t33`
Expected: `6 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Confirm host strict + AVR compile**
```bash
cd /Users/man.nk/git/clibs/avr-libs
cc -std=c11 -Wall -Wextra -Werror -It33Soldering -c t33Soldering/T33Soldering.c -o /dev/null && echo HOST_OK
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c t33Soldering/T33Soldering.c -o /dev/null && echo AVR_OK
```
Expected: `HOST_OK` and `AVR_OK`.

- [ ] **Step 7: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add t33Soldering
git commit -m "refactor(avr-libs/t33Soldering): host-buildable, rename to t33_vol_to_temp, add tests

Drop <avr/io.h>, remove dead code, unique symbol+guard (de-collide from goot80)."
```

---

## Task 5: `ringBuffer` — rewrite to malloc-free byte FIFO + tests

Replace the malloc-backed, busy-waiting, `#define uint8_t char` ring buffer with a clean caller-storage byte FIFO. Also delete the old ad-hoc `ringBuffer/test.c`.

**Files:** Rewrite `ringBuffer/ringbuff.h`, `ringBuffer/ringbuff.c`. Create `ringBuffer/test_ringbuf.c`. Delete `ringBuffer/test.c`.

- [ ] **Step 1: Write the failing test**

Create `ringBuffer/test_ringbuf.c`:
```c
#include "unity.h"
#include "ringbuff.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_fifo_order(void) {
    uint8_t store[4]; ringbuf rb;
    ringbuf_init(&rb, store, 4, false);
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, 10));
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, 20));
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out)); TEST_ASSERT_EQUAL_UINT8(10, out);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out)); TEST_ASSERT_EQUAL_UINT8(20, out);
    TEST_ASSERT_EQUAL_INT(RB_EMPTY, ringbuf_get(&rb, &out));
}

static void test_full_reject_no_infinite_loop(void) {
    /* regression: the old ringBuff_put busy-waited forever when full. */
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 1); ringbuf_put(&rb, 2);
    TEST_ASSERT_TRUE(ringbuf_is_full(&rb));
    TEST_ASSERT_EQUAL_INT(RB_FULL, ringbuf_put(&rb, 3)); /* returns, does NOT hang */
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));
}

static void test_overwrite_evicts_oldest(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, true);
    for (uint8_t i = 1; i <= 3; i++) ringbuf_put(&rb, i);
    TEST_ASSERT_EQUAL_INT(RB_OVERWRITE, ringbuf_put(&rb, 4)); /* drops 1 */
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_wraparound(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, false);
    uint8_t out;
    ringbuf_put(&rb, 1); ringbuf_put(&rb, 2);
    ringbuf_get(&rb, &out);          /* drop 1 */
    ringbuf_put(&rb, 3); ringbuf_put(&rb, 4);   /* wraps */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_peek(void) {
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 99);
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_peek(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(99, out);
    TEST_ASSERT_EQUAL_UINT16(1, ringbuf_count(&rb));
}

static void test_put_string(void) {
    uint8_t store[8]; ringbuf rb;
    ringbuf_init(&rb, store, 8, false);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put_string(&rb, "abc"));
    TEST_ASSERT_EQUAL_UINT16(3, ringbuf_count(&rb));
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('a', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('b', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('c', out);
}

static void test_get_bytes_returns_count(void) {
    /* regression: the old getBytes had a wrong success condition. */
    uint8_t store[8]; ringbuf rb;
    ringbuf_init(&rb, store, 8, false);
    for (uint8_t i = 1; i <= 5; i++) ringbuf_put(&rb, i);
    uint8_t out[10];
    TEST_ASSERT_EQUAL_UINT16(3, ringbuf_get_bytes(&rb, out, 3)); /* copies 3 */
    TEST_ASSERT_EQUAL_UINT8(1, out[0]);
    TEST_ASSERT_EQUAL_UINT8(3, out[2]);
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_get_bytes(&rb, out, 10)); /* only 2 left */
    TEST_ASSERT_EQUAL_UINT8(4, out[0]);
    TEST_ASSERT_EQUAL_UINT8(5, out[1]);
}

static void test_invalid_args(void) {
    ringbuf rb; uint8_t store[1];
    ringbuf_init(&rb, store, 1, false);
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(NULL, 1));
    /* bad init -> safe invalid state */
    ringbuf bad; ringbuf_init(&bad, NULL, 4, false);
    TEST_ASSERT_TRUE(ringbuf_is_empty(&bad));
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(&bad, 1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_full_reject_no_infinite_loop);
    RUN_TEST(test_overwrite_evicts_oldest);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_peek);
    RUN_TEST(test_put_string);
    RUN_TEST(test_get_bytes_returns_count);
    RUN_TEST(test_invalid_args);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make ringbuf`
Expected: FAIL — old `ringbuff.h` has no `ringbuf_init`/`ringbuf` type (it has `ringBuff`/`ringBuff_init`), so compile errors.

- [ ] **Step 3: Rewrite the header**

Replace the entire contents of `ringBuffer/ringbuff.h` with:
```c
/*
 * ringbuff.h — malloc-free byte FIFO over caller-provided storage.
 * NOT reentrant: if shared with an ISR, guard calls with ATOMIC_BLOCK/cli.
 */
#ifndef CLIBS_RINGBUFF_H
#define CLIBS_RINGBUFF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;

typedef struct {
    uint8_t *buf;       /* caller storage, >= cap bytes */
    uint16_t cap;       /* capacity in bytes */
    uint16_t head;      /* index of oldest byte */
    uint16_t count;     /* bytes stored */
    bool     overwrite; /* when full: evict oldest (true) vs reject (false) */
} ringbuf;

void     ringbuf_init(ringbuf *self, uint8_t *storage, uint16_t cap, bool overwrite);
rb_err   ringbuf_put(ringbuf *self, uint8_t byte);
rb_err   ringbuf_get(ringbuf *self, uint8_t *out);    /* out may be NULL to drop */
rb_err   ringbuf_peek(const ringbuf *self, uint8_t *out);
rb_err   ringbuf_put_string(ringbuf *self, const char *s); /* '\0'-terminated */
uint16_t ringbuf_get_bytes(ringbuf *self, uint8_t *out, uint16_t max); /* #copied */
uint16_t ringbuf_count(const ringbuf *self);
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full(const ringbuf *self);
void     ringbuf_clear(ringbuf *self);

#endif /* CLIBS_RINGBUFF_H */
```

- [ ] **Step 4: Rewrite the implementation**

Replace the entire contents of `ringBuffer/ringbuff.c` with:
```c
/*
 * ringbuff.c — malloc-free byte FIFO over caller-provided storage.
 */
#include "ringbuff.h"

void ringbuf_init(ringbuf *self, uint8_t *storage, uint16_t cap, bool overwrite) {
    if (self == NULL) return;
    if (storage == NULL || cap == 0) {     /* invalid config: safe empty state */
        self->buf = NULL; self->cap = 0; self->head = 0; self->count = 0;
        self->overwrite = false;
        return;
    }
    self->buf = storage;
    self->cap = cap;
    self->head = 0;
    self->count = 0;
    self->overwrite = overwrite;
}

rb_err ringbuf_put(ringbuf *self, uint8_t byte) {
    if (self == NULL || self->buf == NULL || self->cap == 0) return RB_INVALID;
    rb_err rc = RB_OK;
    if (self->count == self->cap) {
        if (!self->overwrite) return RB_FULL;
        self->head = (uint16_t)((self->head + 1) % self->cap); /* evict oldest */
        self->count--;
        rc = RB_OVERWRITE;
    }
    uint16_t tail = (uint16_t)(((uint32_t)self->head + self->count) % self->cap);
    self->buf[tail] = byte;
    self->count++;
    return rc;
}

rb_err ringbuf_get(ringbuf *self, uint8_t *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL) *out = self->buf[self->head];
    self->head = (uint16_t)((self->head + 1) % self->cap);
    self->count--;
    return RB_OK;
}

rb_err ringbuf_peek(const ringbuf *self, uint8_t *out) {
    if (self == NULL || self->buf == NULL) return RB_INVALID;
    if (self->count == 0) return RB_EMPTY;
    if (out != NULL) *out = self->buf[self->head];
    return RB_OK;
}

rb_err ringbuf_put_string(ringbuf *self, const char *s) {
    if (self == NULL || self->buf == NULL || s == NULL) return RB_INVALID;
    rb_err rc = RB_OK;
    for (; *s != '\0'; s++) {
        rb_err r = ringbuf_put(self, (uint8_t)*s);
        if (r == RB_FULL) return RB_FULL;
        if (r == RB_OVERWRITE) rc = RB_OVERWRITE;
    }
    return rc;
}

uint16_t ringbuf_get_bytes(ringbuf *self, uint8_t *out, uint16_t max) {
    if (self == NULL || self->buf == NULL || out == NULL) return 0;
    uint16_t n = 0;
    while (n < max && self->count > 0) {
        out[n] = self->buf[self->head];
        self->head = (uint16_t)((self->head + 1) % self->cap);
        self->count--;
        n++;
    }
    return n;
}

uint16_t ringbuf_count(const ringbuf *self)    { return self ? self->count : 0; }
bool     ringbuf_is_empty(const ringbuf *self) { return self ? self->count == 0 : true; }
bool     ringbuf_is_full(const ringbuf *self)  { return self ? self->count == self->cap : true; }

void ringbuf_clear(ringbuf *self) {
    if (self == NULL) return;
    self->head = 0;
    self->count = 0;
}
```

- [ ] **Step 5: Delete the old ad-hoc test**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git rm ringBuffer/test.c
```

- [ ] **Step 6: Run to verify it passes**

Run: `cd /Users/man.nk/git/clibs/avr-libs && make ringbuf`
Expected: `8 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 7: Confirm host strict + AVR compile**
```bash
cd /Users/man.nk/git/clibs/avr-libs
cc -std=c11 -Wall -Wextra -Werror -IringBuffer -c ringBuffer/ringbuff.c -o /dev/null && echo HOST_OK
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c ringBuffer/ringbuff.c -o /dev/null && echo AVR_OK
```
Expected: `HOST_OK` and `AVR_OK`.

- [ ] **Step 8: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add ringBuffer/ringbuff.h ringBuffer/ringbuff.c ringBuffer/test_ringbuf.c ringBuffer/test.c
git commit -m "refactor(avr-libs/ringBuffer): malloc-free byte FIFO over caller storage + tests

Fix: #define uint8_t char, getBytes success condition, infinite busy-wait on
full, malloc usage, fake isBusy lock, putString NUL check."
```

---

## Task 6: Final gates + README touch-up

**Files:** Modify `README.md` (avr-libs) — light updates only.

- [ ] **Step 1: Run all gates**
```bash
cd /Users/man.nk/git/clibs/avr-libs
make clean && make test        # 4 suites: pid 5, goot80 6, t33 6, ringbuf 8 — all 0 Failures
make strict                    # strict: OK
make avr                       # avr: OK
```
Paste the four suite summary lines + the `strict: OK` + `avr: OK`.

- [ ] **Step 2: Update README module notes**

In `README.md`, update the module descriptions to reflect this cycle (find the relevant lines and edit):
- ringBuffer bullet → note it is now a **malloc-free byte FIFO over caller storage** (`ringbuf_*` API).
- goot80_typeK / t33Soldering bullets → note the distinct functions `goot80_vol_to_temp` / `t33_vol_to_temp` (no longer a colliding `volToTemp`).
- Add a short "Tested modules" line: "Pure-logic modules (`myPid`, `goot80_typeK`, `t33Soldering`, `ringBuffer`) have host Unity tests — run `make test`, `make strict`, `make avr`."

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add README.md
git commit -m "docs(avr-libs): note cycle-2a tested modules + API/name changes"
```

- [ ] **Step 4: STOP — controller handles integration.** Do NOT merge/push. Report DONE so the controller can: run the final review, merge the feature branch in avr-libs, push avr-libs, then bump + commit the gitlink in the parent clibs repo.

---

## Self-Review notes

- **Spec coverage:** Unity-in-avr-libs + Makefile gates → Task 1; myPid tests → Task 2; goot80 refactor+rename+stdint+tests → Task 3; t33 same → Task 4; ringBuffer rewrite (all 6 listed bugs) → Task 5; `make test`/`strict`/`avr` + README → Task 6. Bug #7 (avr/io.h) fixed in T3/T4; bug #8 (volToTemp/SOLDERING_H_ collision) fixed via the renames + `CLIBS_*` guards in T3/T4.
- **Type/name consistency:** `ringbuf`/`rb_err`/`ringbuf_*` used identically in header, impl, and test (Task 5). `goot80_vol_to_temp` / `t33_vol_to_temp` consistent across header/impl/test. Makefile module paths match the real folders (`myPid/PID.c`, `goot80_typeK/typeK.c`, `t33Soldering/T33Soldering.c`, `ringBuffer/ringbuff.c`).
- **Placeholder scan:** none — every code/step is complete.
- **Characterization-test note:** Task 2 has no RED (it documents already-correct code); Tasks 3-5 are proper RED→GREEN.
- **Out of scope (untouched):** register drivers, RTOS, max6675, the 4 driver-vendored ringbuff copies, Eclipse metadata.
