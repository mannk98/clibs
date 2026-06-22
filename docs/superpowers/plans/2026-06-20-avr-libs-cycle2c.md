# avr-libs Cycle 2c (Shared led7seg Font) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the duplicated 7-segment font table into one shared, host-Unity-tested `led7seg_font` unit and migrate all three led7seg variants to use it (3 copies → 1).

**Architecture:** A freestanding `led7seg_font` unit (`uint8_t led7seg_segments(uint8_t)`) host-tested with Unity + AVR-compile-gated; the three driver variants replace their local `Led7seg[10]` table with calls to it. Only the font lookup is shared — decompose loops, `binary.h` display constants, and SPI/shiftOut transport are untouched.

**Tech Stack:** C11/C99, Unity (vendored in `avr-libs/third_party/unity/`), GNU make, host `cc`, `avr-gcc` 9.5 at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc`.

**Spec:** `clibs/docs/superpowers/specs/2026-06-20-avr-libs-cycle2c-led7seg-font-design.md`

## Global Constraints
- All work is inside the **`avr-libs` submodule** at `/Users/man.nk/git/clibs/avr-libs/` (repo `mannk98/avr-libs`). Branch created by the controller; implementers must NOT switch branches.
- Shared unit is **freestanding**: `#include <stdint.h>` only. Header guard `CLIBS_LED7SEG_FONT_H`.
- Font values are **standard hex literals** (no `binary.h` `B*` macros) and must be byte-identical to the originals: `0xC0,0xCF,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90`; out-of-range digit → `0xFF`.
- Migration is font-only: replace `Led7seg[...]` with `led7seg_segments(...)`, delete the local table; leave decompose loops, `binary.h` constants, and transport untouched (zero behavior change).
- Append to every commit message:
  ```
  Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
  ```

## File Structure

| File | Responsibility |
|------|----------------|
| `led7seg_font/led7seg_font.{h,c}` | shared font lookup (new) |
| `led7seg_font/test_led7seg_font.c` | Unity suite (new) |
| `Makefile` | (modify) add `led7seg` host-test target + fold into `test`/`strict`/`avr` |
| `led7segShiftout/led7seg.c` | (modify) use shared font |
| `led7segSpi/led7seg.c` | (modify) use shared font |
| `led7segSpi(noISR)/led7seg.c` | (modify) use shared font |

---

## Task 1: `led7seg_font` unit + Makefile target

**Files:**
- Create: `led7seg_font/led7seg_font.h`, `led7seg_font/led7seg_font.c`, `led7seg_font/test_led7seg_font.c`
- Modify: `Makefile`

**Interfaces:**
- Produces: `uint8_t led7seg_segments(uint8_t digit)` — 0-9 → segment byte; >9 → `0xFF`.

- [ ] **Step 1: Add the Makefile target.** The Makefile already has targets through cycle 2b. Make these edits:

  Edit the `.PHONY` line — add `led7seg`:
  ```
  .PHONY: test pid goot80 t33 ringbuf max6675 encoder wallclock led7seg strict avr clean
  ```
  Edit the `test:` line — add `led7seg`:
  ```
  test: pid goot80 t33 ringbuf max6675 encoder wallclock led7seg
  ```
  Insert this target block immediately after the `wallclock:` target block (recipe lines are TABs):
  ```makefile
  led7seg: | $(BUILD)
  	$(CC) $(CFLAGS) -I$(UNITY) -Iled7seg_font -o $(BUILD)/test_led7seg \
  	    led7seg_font/test_led7seg_font.c led7seg_font/led7seg_font.c $(UNITY)/unity.c
  	./$(BUILD)/test_led7seg
  ```
  In the `strict:` target, add this line immediately before its `@echo "strict: OK"`:
  ```makefile
  	$(CC) -std=c11 -Wall -Wextra -Werror -Iled7seg_font -c led7seg_font/led7seg_font.c -o /dev/null
  ```
  In the `avr:` target, add this line immediately before its `@echo "avr: OK"`:
  ```makefile
  	$(AVR_CC) $(AVRFLAGS) -c led7seg_font/led7seg_font.c -o /dev/null
  ```

- [ ] **Step 2: Write the failing test** — Create `led7seg_font/test_led7seg_font.c`:
```c
#include "unity.h"
#include "led7seg_font.h"

void setUp(void) {}
void tearDown(void) {}

/* Font bytes are bit-identical to the original B-macro table:
 * B11000000=0xC0 ... B10010000=0x90 (common-anode, 74HC595). */
static void test_digit_font(void) {
    TEST_ASSERT_EQUAL_HEX8(0xC0, led7seg_segments(0));
    TEST_ASSERT_EQUAL_HEX8(0xCF, led7seg_segments(1));
    TEST_ASSERT_EQUAL_HEX8(0xA4, led7seg_segments(2));
    TEST_ASSERT_EQUAL_HEX8(0xB0, led7seg_segments(3));
    TEST_ASSERT_EQUAL_HEX8(0x99, led7seg_segments(4));
    TEST_ASSERT_EQUAL_HEX8(0x92, led7seg_segments(5));
    TEST_ASSERT_EQUAL_HEX8(0x82, led7seg_segments(6));
    TEST_ASSERT_EQUAL_HEX8(0xF8, led7seg_segments(7));
    TEST_ASSERT_EQUAL_HEX8(0x80, led7seg_segments(8));
    TEST_ASSERT_EQUAL_HEX8(0x90, led7seg_segments(9));
}
static void test_out_of_range_is_off(void) {
    TEST_ASSERT_EQUAL_HEX8(0xFF, led7seg_segments(10));
    TEST_ASSERT_EQUAL_HEX8(0xFF, led7seg_segments(255));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_digit_font);
    RUN_TEST(test_out_of_range_is_off);
    return UNITY_END();
}
```

- [ ] **Step 3: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/avr-libs && make led7seg`
Expected: compile/link error — `led7seg_font.h` / `led7seg_segments` don't exist.

- [ ] **Step 4: Create `led7seg_font/led7seg_font.h`**
```c
#ifndef CLIBS_LED7SEG_FONT_H
#define CLIBS_LED7SEG_FONT_H

#include <stdint.h>

/* 7-segment font for a common-anode display driven through a 74HC595.
 * Returns the segment byte for a single decimal digit 0-9.
 * digit > 9 -> 0xFF (all segments off). */
uint8_t led7seg_segments(uint8_t digit);

#endif /* CLIBS_LED7SEG_FONT_H */
```

- [ ] **Step 5: Create `led7seg_font/led7seg_font.c`**
```c
#include "led7seg_font.h"

/* bit-identical to the original B-macro table (B11000000 == 0xC0, ...) */
static const uint8_t LED7SEG_FONT[10] = {
    0xC0, /* 0 */
    0xCF, /* 1 */
    0xA4, /* 2 */
    0xB0, /* 3 */
    0x99, /* 4 */
    0x92, /* 5 */
    0x82, /* 6 */
    0xF8, /* 7 */
    0x80, /* 8 */
    0x90, /* 9 */
};

uint8_t led7seg_segments(uint8_t digit) {
    if (digit > 9)
        return 0xFF; /* all segments off */
    return LED7SEG_FONT[digit];
}
```

- [ ] **Step 6: Run, expect PASS** — `cd /Users/man.nk/git/clibs/avr-libs && make led7seg`
Expected: `2 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 7: Confirm host-strict + AVR compile of the unit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
cc -std=c11 -Wall -Wextra -Werror -Iled7seg_font -c led7seg_font/led7seg_font.c -o /dev/null && echo HOST_OK
/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c led7seg_font/led7seg_font.c -o /dev/null && echo AVR_OK
```
Expected: `HOST_OK` and `AVR_OK`.

- [ ] **Step 8: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add Makefile led7seg_font/led7seg_font.h led7seg_font/led7seg_font.c led7seg_font/test_led7seg_font.c
git commit -m "feat(avr-libs/led7seg_font): shared host-tested 7-seg font lookup"
```

---

## Task 2: Migrate all three led7seg variants to the shared font

The three variants (`led7segShiftout/led7seg.c`, `led7segSpi/led7seg.c`, `led7segSpi(noISR)/led7seg.c`) each declare an identical `const uint8_t Led7seg[10]` table and use `Led7seg[value]` / `Led7seg[value % 10]`. This is one mechanical, identical edit applied to all three.

**Files:**
- Modify: `led7segShiftout/led7seg.c`, `led7segSpi/led7seg.c`, `led7segSpi(noISR)/led7seg.c`

**Interfaces:**
- Consumes: `uint8_t led7seg_segments(uint8_t digit)` from `led7seg_font/led7seg_font.h` (Task 1).

- [ ] **Step 1: Apply the same three edits to each of the three files**

  For **each** of `led7segShiftout/led7seg.c`, `led7segSpi/led7seg.c`, `led7segSpi(noISR)/led7seg.c`:

  (a) Add the include immediately after the existing `#include "led7seg.h"`:
  ```c
  #include "led7seg_font.h"
  ```
  (b) Delete the entire local font table definition — the block that starts with
  `const uint8_t Led7seg[10] = {` and ends at the closing `};` (the 10 `B*` entries).
  (c) Replace every occurrence of `Led7seg[value % 10]` with `led7seg_segments(value % 10)`,
  and every occurrence of `Led7seg[value]` with `led7seg_segments(value)`.

  (No other lines change — decompose loops, `common` shifting, modes, `binary.h` display
  constants like `B00001000`/`B11111111`, and the `spiTradeByte*`/`shiftOut` calls stay exactly
  as they are. `binary.h` is still included by each file for those display constants — keep it.)

- [ ] **Step 2: Verify the substitution is complete in all three files**
```bash
cd /Users/man.nk/git/clibs/avr-libs
echo "=== should be EMPTY (no Led7seg identifier left) ==="
grep -rn 'Led7seg' led7segShiftout/led7seg.c led7segSpi/led7seg.c "led7segSpi(noISR)/led7seg.c"
echo "=== led7seg_segments call counts (should be >0 each) ==="
grep -cn 'led7seg_segments' led7segShiftout/led7seg.c led7segSpi/led7seg.c "led7segSpi(noISR)/led7seg.c"
echo "=== include present in each ==="
grep -l 'led7seg_font.h' led7segShiftout/led7seg.c led7segSpi/led7seg.c "led7segSpi(noISR)/led7seg.c"
```
Expected: the first grep prints NOTHING (capital-L `Led7seg` table + index uses all gone); the count grep shows a nonzero count for each file; all three filenames listed for the include.

- [ ] **Step 3: Re-run the unit suite + strict/avr gate for the font unit (unchanged, must stay green)**
```bash
cd /Users/man.nk/git/clibs/avr-libs && make led7seg && make strict 2>&1 | tail -1
```
Expected: `2 Tests 0 Failures 0 Ignored` / `OK` and `strict: OK`.

- [ ] **Step 4: Best-effort AVR compile of each migrated driver** (the vendored `binary.h`/`spi.h` soup may block it — that is acceptable and expected; report which happened per file):
```bash
cd /Users/man.nk/git/clibs/avr-libs
for d in led7segShiftout led7segSpi "led7segSpi(noISR)"; do
  if /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall \
       -Iled7seg_font -I"$d" -c "$d/led7seg.c" -o /dev/null 2>/tmp/e; then
    echo "DRIVER_AVR_OK: $d"
  else
    echo "DRIVER_AVR_BLOCKED: $d (vendored include soup — deferred to dedup cycle)"; head -3 /tmp/e
  fi
done
```

- [ ] **Step 5: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add led7segShiftout/led7seg.c led7segSpi/led7seg.c "led7segSpi(noISR)/led7seg.c"
git commit -m "refactor(avr-libs/led7seg): use shared led7seg_font; drop 3x duplicated font table"
```

---

## Task 3: Final gates + README note

**Files:** Modify `README.md`.

- [ ] **Step 1: Run all gates**
```bash
cd /Users/man.nk/git/clibs/avr-libs
make clean && make test   # 8 suites: pid 5, goot80 6, t33 6, ringbuf 12, max6675 3, encoder 3, wallclock 3, led7seg 2 — all 0 Failures
make strict               # strict: OK
make avr                  # avr: OK
```
Paste the eight suite summary lines + `strict: OK` + `avr: OK`. If anything fails, STOP and report BLOCKED.

- [ ] **Step 2: Update `README.md`** — in the `## Testing` section, add `led7seg_font` to the host-tested list and `led7seg 2` to the `make test` counts. Then READ the `## Conventions & notes` "Shared helpers are vendored (copied) per module" bullet and update it to reflect that the **7-seg font is now shared** (`led7seg_font/`), while `binary.h` and the pin-mapping copies remain per-module. Keep everything else intact; match the existing markdown style.

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs/avr-libs
git add README.md
git commit -m "docs(avr-libs): note shared led7seg_font + cycle-2c test counts"
```

- [ ] **Step 4: STOP — controller handles integration.** Do NOT merge/push. Report DONE so the controller runs the final review, merges the feature branch in avr-libs, pushes avr-libs, then bumps + commits the gitlink in clibs.

---

## Self-Review notes
- **Spec coverage:** shared unit (hex font, 0xFF OOB, guard) + host tests → Task 1; Makefile `led7seg`/`strict`/`avr` → Task 1; migrate all 3 variants (font-only, grep-verified) → Task 2; final gates + README → Task 3. Best-effort driver AVR compile per the spec → Task 2 Step 4.
- **Interfaces / type consistency:** `led7seg_segments(uint8_t)` identical across the header, impl, test, and the three call sites.
- **Placeholder scan:** none.
- **Behavior preservation:** font hex == original `B*` bytes; migration only swaps the table for a call; no decompose/transport/`binary.h` change.
- **Out of scope (untouched):** decompose loops, `binary.h`/pin-mapping dedup, ringbuff driver dedup (wants simavr), all other modules.
