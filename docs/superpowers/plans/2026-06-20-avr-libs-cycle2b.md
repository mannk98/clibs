# avr-libs Cycle 2b (Driver Logic Extraction) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the pure algorithmic core of `max6675`, `encoder`, and `wallClock` into hardware-free sub-units that are host-Unity-tested + AVR-compile-gated, wiring the existing drivers to call them (and fixing the `readFah` bug).

**Architecture:** Each module gains a hardware-free pure sub-unit (`max6675_conv`, `encoder_decode`, `wallclock_tick`) host-tested with Unity; the existing driver becomes a thin wrapper that calls it (DRY). The vendored-header driver compile stays out of scope.

**Tech Stack:** C11/C99, Unity (already vendored in `avr-libs/third_party/unity/`), GNU make, host `cc`, `avr-gcc` 9.5 at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc`.

**Spec:** `clibs/docs/superpowers/specs/2026-06-20-avr-libs-cycle2b-logic-extraction-design.md`

## Global Constraints
- All work is inside the **`avr-libs` submodule** on disk at `/Users/man.nk/git/clibs/avr-libs/` (its own repo `mannk98/avr-libs`). Every `git` command runs there. Branch is created by the controller; implementers must NOT switch branches.
- New sub-units are **freestanding**: `#include <stdint.h>` only (`wallclock_tick` additionally relies on a compile-time `-DF_CPU=16000000UL`). No `<avr/io.h>`, no `<util/delay.h>`, no hardware headers in the sub-units.
- Header guards: `CLIBS_MAX6675_CONV_H`, `CLIBS_ENCODER_DECODE_H`, `CLIBS_WALLCLOCK_TICK_H`.
- Conversion/decoding math preserved exactly; the only behavior change is fixing `readFah`.
- Append to every commit message:
  ```
  Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
  ```

## File Structure

| File | Responsibility |
|------|----------------|
| `Makefile` | (modify) add `max6675`/`encoder`/`wallclock` host-test targets + fold into `test`/`strict`/`avr` |
| `max6675/max6675_conv.{h,c}` | pure raw→°C + °C→°F (new) |
| `max6675/test_max6675_conv.c` | Unity suite (new) |
| `max6675/max6675.{c,h}` | (modify) call conv unit; fix readFah; drop NAN macro |
| `encoder/encoder_decode.{h,c}` | pure quadrature decode (new) |
| `encoder/test_encoder_decode.c` | Unity suite (new) |
| `encoder/encoder.c` | (modify) read pins → call decode |
| `wallClock/wallclock_tick.{h,c}` | pure overflow accumulator (new) |
| `wallClock/wallCLock.c` | (modify) ISR calls wallclock_tick |

---

## Task 1: `max6675_conv` + Makefile targets + wire driver

**Files:**
- Modify: `Makefile`
- Create: `max6675/max6675_conv.h`, `max6675/max6675_conv.c`, `max6675/test_max6675_conv.c`
- Modify: `max6675/max6675.c`, `max6675/max6675.h`

**Interfaces:**
- Produces: `float max6675_raw_to_celsius(uint16_t raw)`, `float max6675_celsius_to_fahrenheit(float c)`, `#define MAX6675_OPEN (-100.0f)`.

- [ ] **Step 1: Update the Makefile** — replace the ENTIRE `Makefile` with (recipe lines MUST be real TABs):
```makefile
# Host build + test runner + AVR cross-compile gate for avr-libs.
CC       ?= cc
AVR_CC   ?= /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc
CFLAGS   := -std=c11 -Wall -Wextra -g
AVRFLAGS := -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra
UNITY    := third_party/unity
BUILD    := build

.PHONY: test pid goot80 t33 ringbuf max6675 encoder wallclock strict avr clean
test: pid goot80 t33 ringbuf max6675 encoder wallclock

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

max6675: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imax6675 -o $(BUILD)/test_max6675 \
	    max6675/test_max6675_conv.c max6675/max6675_conv.c $(UNITY)/unity.c
	./$(BUILD)/test_max6675

encoder: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iencoder -o $(BUILD)/test_encoder \
	    encoder/test_encoder_decode.c encoder/encoder_decode.c $(UNITY)/unity.c
	./$(BUILD)/test_encoder

wallclock: | $(BUILD)
	$(CC) $(CFLAGS) -DF_CPU=16000000UL -I$(UNITY) -IwallClock -o $(BUILD)/test_wallclock \
	    wallClock/test_wallclock_tick.c wallClock/wallclock_tick.c $(UNITY)/unity.c
	./$(BUILD)/test_wallclock

strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -ImyPid        -c myPid/PID.c                -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Igoot80_typeK -c goot80_typeK/typeK.c        -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -It33Soldering -c t33Soldering/T33Soldering.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -IringBuffer   -c ringBuffer/ringbuff.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imax6675      -c max6675/max6675_conv.c      -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iencoder      -c encoder/encoder_decode.c    -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -DF_CPU=16000000UL -IwallClock -c wallClock/wallclock_tick.c -o /dev/null
	@echo "strict: OK"

avr:
	$(AVR_CC) $(AVRFLAGS) -c myPid/PID.c                 -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c goot80_typeK/typeK.c        -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c t33Soldering/T33Soldering.c -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c ringBuffer/ringbuff.c       -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c max6675/max6675_conv.c      -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c encoder/encoder_decode.c    -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c wallClock/wallclock_tick.c  -o /dev/null
	@echo "avr: OK"

clean:
	rm -rf $(BUILD)
```

- [ ] **Step 2: Write the failing test** — Create `max6675/test_max6675_conv.c`:
```c
#include "unity.h"
#include "max6675_conv.h"

void setUp(void) {}
void tearDown(void) {}

static void test_open_thermocouple(void) {
    /* bit 2 (0x4) set => open => MAX6675_OPEN */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, MAX6675_OPEN, max6675_raw_to_celsius(0x0004));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, MAX6675_OPEN, max6675_raw_to_celsius(0x0C84));
}
static void test_raw_to_celsius(void) {
    /* raw>>3 then *0.25; 0x0C80 (bit2 clear) -> 400*0.25 = 100.0 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 100.0f, max6675_raw_to_celsius(0x0C80));
    /* 0x08 -> 1*0.25 = 0.25 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.25f, max6675_raw_to_celsius(0x0008));
    /* 0x00 -> 0 */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, max6675_raw_to_celsius(0x0000));
}
static void test_celsius_to_fahrenheit(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 32.0f,  max6675_celsius_to_fahrenheit(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 212.0f, max6675_celsius_to_fahrenheit(100.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, -40.0f, max6675_celsius_to_fahrenheit(-40.0f));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_open_thermocouple);
    RUN_TEST(test_raw_to_celsius);
    RUN_TEST(test_celsius_to_fahrenheit);
    return UNITY_END();
}
```

- [ ] **Step 3: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/avr-libs && make max6675`
Expected: compile/link error — `max6675_conv.h` / functions don't exist yet.

- [ ] **Step 4: Create `max6675/max6675_conv.h`**
```c
#ifndef CLIBS_MAX6675_CONV_H
#define CLIBS_MAX6675_CONV_H

#include <stdint.h>

/* Returned by max6675_raw_to_celsius when the thermocouple is open (raw bit 2 set). */
#define MAX6675_OPEN (-100.0f)

/* Convert a raw 16-bit MAX6675 reading to degrees Celsius.
 * If bit 2 is set the thermocouple is open -> returns MAX6675_OPEN.
 * Otherwise the temperature is (raw >> 3) * 0.25 degC. */
float max6675_raw_to_celsius(uint16_t raw);

/* Standard Celsius -> Fahrenheit. */
float max6675_celsius_to_fahrenheit(float c);

#endif /* CLIBS_MAX6675_CONV_H */
```

- [ ] **Step 5: Create `max6675/max6675_conv.c`**
```c
#include "max6675_conv.h"

float max6675_raw_to_celsius(uint16_t raw) {
    if (raw & 0x4)            /* bit 2 set => no thermocouple attached */
        return MAX6675_OPEN;
    raw >>= 3;
    return raw * 0.25f;
}

float max6675_celsius_to_fahrenheit(float c) {
    return c * 9.0f / 5.0f + 32.0f;
}
```

- [ ] **Step 6: Run, expect PASS** — `cd /Users/man.nk/git/clibs/avr-libs && make max6675`
Expected: `3 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 7: Wire the driver — `max6675/max6675.c`.** Add the include after `#include "max6675.h"`:
```c
#include "max6675_conv.h"
```
Replace the body of `readCel` (keep the SPI read; replace only the conversion tail) so the function reads:
```c
float readCel(const max6675 self) {
	uint16_t v;

	digitalWrite(self->cs, LOW);
	delayMicroseconds(10);

	v = self->spiread(self);
	v <<= 8;
	v |= spiread(self);

	digitalWrite(self->cs, HIGH);

	return max6675_raw_to_celsius(v);
}
```
Replace `readFah` (fixes the undefined `readCelsius()` bug):
```c
float readFah(const max6675 self) {
	return max6675_celsius_to_fahrenheit(readCel(self));
}
```

- [ ] **Step 8: Remove the bad macro from `max6675/max6675.h`** — delete the line:
```c
#define NAN -100.0
```
(It is no longer referenced; the open-thermocouple sentinel now lives in `max6675_conv.h` as `MAX6675_OPEN`.)

- [ ] **Step 9: Best-effort AVR compile of the modified driver** (the vendored-header soup may block it — that is acceptable and expected; report which happened):
```bash
cd /Users/man.nk/git/clibs/avr-libs
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall \
  -Imax6675 -Iled7segShiftout -c max6675/max6675.c -o /dev/null 2>&1 | head -5 \
  && echo "DRIVER_AVR_OK" || echo "DRIVER_AVR_BLOCKED (vendored include soup — deferred to dedup cycle)"
```
Re-run the sub-unit AVR gate (must pass): `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c max6675/max6675_conv.c -o /dev/null && echo CONV_AVR_OK`

- [ ] **Step 10: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add Makefile max6675/max6675_conv.h max6675/max6675_conv.c max6675/test_max6675_conv.c max6675/max6675.c max6675/max6675.h
git commit -m "feat(avr-libs/max6675): extract host-tested raw->C/C->F conv; fix readFah bug"
```

---

## Task 2: `encoder_decode` + wire driver

**Files:**
- Create: `encoder/encoder_decode.h`, `encoder/encoder_decode.c`, `encoder/test_encoder_decode.c`
- Modify: `encoder/encoder.c`

**Interfaces:**
- Produces: `void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter)`.

- [ ] **Step 1: Write the failing test** — Create `encoder/test_encoder_decode.c`:
```c
#include "unity.h"
#include "encoder_decode.h"

void setUp(void) {}
void tearDown(void) {}

static void test_cw_detent_increments(void) {
    uint8_t pre_a = 0, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 1, &counter); /* A edge 0->1, a==b, count=1 */
    TEST_ASSERT_EQUAL_INT(0, counter);              /* not yet a full detent */
    encoder_decode(&pre_a, &count, 0, 0, &counter); /* A edge 1->0, a==b, count=2 -> +1 */
    TEST_ASSERT_EQUAL_INT(1, counter);
    TEST_ASSERT_EQUAL_UINT8(0, count);              /* reset */
}
static void test_ccw_detent_decrements(void) {
    uint8_t pre_a = 0, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 0, &counter); /* A edge, a!=b, count=1 */
    encoder_decode(&pre_a, &count, 0, 1, &counter); /* A edge, a!=b, count=2 -> -1 */
    TEST_ASSERT_EQUAL_INT(-1, counter);
}
static void test_no_a_edge_no_change(void) {
    uint8_t pre_a = 1, count = 0; int counter = 0;
    encoder_decode(&pre_a, &count, 1, 0, &counter); /* A unchanged (1==1) */
    TEST_ASSERT_EQUAL_INT(0, counter);
    TEST_ASSERT_EQUAL_UINT8(0, count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cw_detent_increments);
    RUN_TEST(test_ccw_detent_decrements);
    RUN_TEST(test_no_a_edge_no_change);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/avr-libs && make encoder`
Expected: compile/link error — `encoder_decode.h` / function don't exist.

- [ ] **Step 3: Create `encoder/encoder_decode.h`**
```c
#ifndef CLIBS_ENCODER_DECODE_H
#define CLIBS_ENCODER_DECODE_H

#include <stdint.h>

/* Quadrature decode for one poll. a/b are the just-read A/B pin states.
 * Tracks the previous A state (*pre_a) and a half-detent counter (*count);
 * on every 2nd A-edge (a full detent) applies +1 (a==b) or -1 (a!=b) to *counter. */
void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter);

#endif /* CLIBS_ENCODER_DECODE_H */
```

- [ ] **Step 4: Create `encoder/encoder_decode.c`**
```c
#include "encoder_decode.h"

void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter) {
    if (a != *pre_a) {
        (*count)++;
        if (*count == 2) {
            if (a == b) (*counter)++;
            else        (*counter)--;
            *count = 0;
        }
        *pre_a = a;
    }
}
```

- [ ] **Step 5: Run, expect PASS** — `cd /Users/man.nk/git/clibs/avr-libs && make encoder`
Expected: `3 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 6: Wire the driver — `encoder/encoder.c`.** Add after `#include "encoder.h"`:
```c
#include "encoder_decode.h"
```
Replace the entire `encoderCheck` function with:
```c
void encoderCheck(encoder *self, void *counter) { // pass address of itself
	uint8_t pinA_State = digitalRead(self->pinA);
	uint8_t pinB_State = digitalRead(self->pinB);

	encoder_decode(&self->pre_pinA_State, &self->count,
	               pinA_State, pinB_State, (int *)counter);
}
```

- [ ] **Step 7: Best-effort AVR compile of the modified driver + sub-unit AVR gate:**
```bash
cd /Users/man.nk/git/clibs/avr-libs
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall \
  -Iencoder -I. -c encoder/encoder.c -o /dev/null 2>&1 | head -5 \
  && echo "DRIVER_AVR_OK" || echo "DRIVER_AVR_BLOCKED (vendored include soup — deferred to dedup cycle)"
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c encoder/encoder_decode.c -o /dev/null && echo DECODE_AVR_OK
```

- [ ] **Step 8: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add encoder/encoder_decode.h encoder/encoder_decode.c encoder/test_encoder_decode.c encoder/encoder.c
git commit -m "feat(avr-libs/encoder): extract host-tested quadrature decode; wire encoderCheck"
```

---

## Task 3: `wallclock_tick` + wire ISR

**Files:**
- Create: `wallClock/wallclock_tick.h`, `wallClock/wallclock_tick.c`, `wallClock/test_wallclock_tick.c`
- Modify: `wallClock/wallCLock.c`

**Interfaces:**
- Produces: `typedef struct { uint32_t millis; uint8_t fract; } wallclock_accum;` and `void wallclock_tick(wallclock_accum *a)`.

- [ ] **Step 1: Write the failing test** — Create `wallClock/test_wallclock_tick.c`:
```c
#include "unity.h"
#include "wallclock_tick.h"

void setUp(void) {}
void tearDown(void) {}

/* Compiled with -DF_CPU=16000000UL: MILLIS_INC=1, FRACT_INC=3, FRACT_MAX=125. */
static void test_one_tick(void) {
    wallclock_accum a = { 0, 0 };
    wallclock_tick(&a);
    TEST_ASSERT_EQUAL_UINT32(1, a.millis);
    TEST_ASSERT_EQUAL_UINT8(3, a.fract);
}
static void test_thousand_ticks_is_1024ms(void) {
    wallclock_accum a = { 0, 0 };
    for (int i = 0; i < 1000; i++) wallclock_tick(&a);
    TEST_ASSERT_EQUAL_UINT32(1024, a.millis); /* 1000 * 1.024 ms */
    TEST_ASSERT_EQUAL_UINT8(0, a.fract);      /* 1000*3 = 3000 = 24*125 exactly */
}
static void test_monotonic(void) {
    wallclock_accum a = { 0, 0 };
    uint32_t prev = 0;
    for (int i = 0; i < 50; i++) {
        wallclock_tick(&a);
        TEST_ASSERT_TRUE(a.millis >= prev);
        prev = a.millis;
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_one_tick);
    RUN_TEST(test_thousand_ticks_is_1024ms);
    RUN_TEST(test_monotonic);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/avr-libs && make wallclock`
Expected: compile/link error — `wallclock_tick.h` / type / function don't exist.

- [ ] **Step 3: Create `wallClock/wallclock_tick.h`**
```c
#ifndef CLIBS_WALLCLOCK_TICK_H
#define CLIBS_WALLCLOCK_TICK_H

#include <stdint.h>

/* Millisecond accumulator advanced once per Timer0 overflow. */
typedef struct {
    uint32_t millis;
    uint8_t  fract;
} wallclock_accum;

/* Advance the accumulator by one Timer0 overflow. Requires F_CPU to be defined
 * at compile time (e.g. -DF_CPU=16000000UL). */
void wallclock_tick(wallclock_accum *a);

#endif /* CLIBS_WALLCLOCK_TICK_H */
```

- [ ] **Step 4: Create `wallClock/wallclock_tick.c`** (constants verbatim from the original `wallCLock.c`; prescaler 64, overflow every 256 counts):
```c
#include "wallclock_tick.h"

/* microseconds per Timer0 overflow = (64 prescaler * 256 counts) / (F_CPU MHz). */
#define MICROSECONDS_PER_TIMER0_OVERFLOW (64 * 256 / (F_CPU / 1000000L))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)        /* 16MHz -> 1 */
#define FRACT_INC  ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3) /* 16MHz -> 24>>3 = 3 */
#define FRACT_MAX  (1000 >> 3)                                      /* 125 */

void wallclock_tick(wallclock_accum *a) {
    a->millis += MILLIS_INC;
    a->fract  += FRACT_INC;
    if (a->fract >= FRACT_MAX) {
        a->fract  -= FRACT_MAX;
        a->millis += 1;
    }
}
```

- [ ] **Step 5: Run, expect PASS** — `cd /Users/man.nk/git/clibs/avr-libs && make wallclock`
Expected: `3 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 6: Wire the ISR — `wallClock/wallCLock.c`.** Add after `#include "wallClock.h"`:
```c
#include "wallclock_tick.h"
```
Replace the body of the `ISR(...)` overflow handler (the block currently doing the `m += MILLIS_INC; f += FRACT_INC; ...` accumulation) with:
```c
{
	wallclock_accum a = { timer0_millis, timer0_fract };
	wallclock_tick(&a);
	timer0_millis = a.millis;
	timer0_fract  = a.fract;
	timer0_overflow_count++;
}
```
(Leave `millis()`, `micros()`, `initWallClock()`, the `timer0_*` globals, and the `clockCyclesPerMicrosecond()` macros untouched. The old `MICROSECONDS_PER_TIMER0_OVERFLOW`/`MILLIS_INC`/`FRACT_INC`/`FRACT_MAX` macros in this file become unused — leave them; they cause no warning and the canonical copy now lives in `wallclock_tick.c`. The stale "24>>3 = 6" comment is corrected in `wallclock_tick.c`.)

- [ ] **Step 7: Best-effort AVR compile of the modified driver + sub-unit AVR gate:**
```bash
cd /Users/man.nk/git/clibs/avr-libs
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall \
  -IwallClock -c wallClock/wallCLock.c -o /dev/null 2>&1 | head -5 \
  && echo "DRIVER_AVR_OK" || echo "DRIVER_AVR_BLOCKED (deferred to dedup cycle)"
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c wallClock/wallclock_tick.c -o /dev/null && echo TICK_AVR_OK
```

- [ ] **Step 8: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add wallClock/wallclock_tick.h wallClock/wallclock_tick.c wallClock/test_wallclock_tick.c wallClock/wallCLock.c
git commit -m "feat(avr-libs/wallClock): extract host-tested millis accumulator; ISR calls wallclock_tick"
```

---

## Task 4: Final gates + README note

**Files:** Modify `README.md`.

- [ ] **Step 1: Run all gates**
```bash
cd /Users/man.nk/git/clibs/avr-libs
make clean && make test   # pid 5, goot80 6, t33 6, ringbuf 12, max6675 3, encoder 3, wallclock 3 — all 0 Failures
make strict               # strict: OK
make avr                  # avr: OK
```
Paste the seven suite summary lines + `strict: OK` + `avr: OK`. If anything fails, STOP and report BLOCKED.

- [ ] **Step 2: Update `README.md`** — in the `## Testing` section, extend the module list to include the three newly-tested sub-units and update the suite counts. First READ the current `## Testing` block, then edit it so it states the host-tested set is `myPid, goot80_typeK, t33Soldering, ringBuffer, max6675 (conv), encoder (decode), wallClock (tick)` and the `make test` line lists the new counts (`… max6675 3, encoder 3, wallclock 3`). Also, in the "Conventions & notes" rough-edges bullet, **remove the now-fixed `max6675.c readFah references an undefined readCelsius()` claim** (it is fixed this cycle). Keep all other content intact; match the existing markdown style.

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add README.md
git commit -m "docs(avr-libs): note cycle-2b extracted+tested driver logic; drop fixed readFah note"
```

- [ ] **Step 4: STOP — controller handles integration.** Do NOT merge/push. Report DONE so the controller runs the final review, merges the feature branch in avr-libs, pushes avr-libs, then bumps + commits the gitlink in clibs.

---

## Self-Review notes
- **Spec coverage:** max6675_conv + readFah fix + drop NAN → Task 1; encoder_decode → Task 2; wallclock_tick (F_CPU, comment fix) → Task 3; Makefile test/strict/avr gates → Task 1; README + final gates → Task 4. Driver-wiring + best-effort-AVR (sub-units gated, drivers best-effort) handled per task.
- **Interfaces / type consistency:** `max6675_raw_to_celsius`/`max6675_celsius_to_fahrenheit`/`MAX6675_OPEN`, `encoder_decode(uint8_t*,uint8_t*,uint8_t,uint8_t,int*)`, `wallclock_accum{millis,fract}`/`wallclock_tick` — names identical across header, impl, test, and the driver call sites.
- **Placeholder scan:** none.
- **Characterization vs RED→GREEN:** Tasks 1-3 are proper RED (sub-unit absent) → GREEN. Math preserved exactly except the readFah bug fix.
- **Out of scope (untouched):** adc, led7seg, RTOS, spi, timer, the vendored `ringbuff`/pin-mapping copies, full driver AVR compilation (best-effort only).
