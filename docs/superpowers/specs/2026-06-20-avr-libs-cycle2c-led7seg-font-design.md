# avr-libs cycle 2c — shared led7seg font (design)

Date: 2026-06-20
Repo: work happens inside the **`avr-libs` git submodule** (`mannk98/avr-libs`, branch `main`).
Planning docs live in the parent `clibs` repo (this file).

## Context

The three 7-segment display variants — `led7segShiftout` (bit-bang), `led7segSpi` (SPI),
`led7segSpi(noISR)` (SPI polling) — each carry a byte-identical `const uint8_t Led7seg[10]`
font table and use it identically (`Led7seg[value % 10]`). This cycle extracts that font into
one shared, host-Unity-tested unit and migrates all three to use it (3 copies → 1).

No-simavr cycle: the font lookup is pure and host-testable. The inline decompose loops and the
SPI/shiftOut transport stay untouched (changing them would restructure register-coupled,
host-untestable display code). Testing stays hybrid: host Unity for the unit + `avr-gcc`
compile-gate. avr-gcc 9.5 is at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc` (keg-only).

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | Shared `led7seg_font` unit + migrate all 3 variants (font lookup only) |
| Font values | **Standard hex literals** in the shared unit (not the non-portable `binary.h` `B*` macros); byte-identical |
| Decompose loops | Left inline in each driver — unchanged behavior (extraction deferred; restructures untested display code) |
| `binary.h` | Stays in each variant (still used for position/`common`/blank display bytes); its dedup is a separate cycle |
| Out of scope | decompose extraction, `binary.h`/pin-mapping dedup, ringbuff driver dedup (wants simavr), SPI transport |

## The shared unit

`led7seg_font/led7seg_font.h` (guard `CLIBS_LED7SEG_FONT_H`, `#include <stdint.h>` only):
```c
/* 7-segment font for a common-anode display driven through a 74HC595.
 * Returns the segment byte for a single decimal digit 0-9; digit > 9 -> 0xFF (all off). */
uint8_t led7seg_segments(uint8_t digit);
```

`led7seg_font/led7seg_font.c`:
```c
#include "led7seg_font.h"

static const uint8_t LED7SEG_FONT[10] = {
    0xC0, /* 0  (was B11000000) */
    0xCF, /* 1  (was B11001111) */
    0xA4, /* 2  (was B10100100) */
    0xB0, /* 3  (was B10110000) */
    0x99, /* 4  (was B10011001) */
    0x92, /* 5  (was B10010010) */
    0x82, /* 6  (was B10000010) */
    0xF8, /* 7  (was B11111000) */
    0x80, /* 8  (was B10000000) */
    0x90, /* 9  (was B10010000) */
};

uint8_t led7seg_segments(uint8_t digit) {
    if (digit > 9) return 0xFF; /* all segments off */
    return LED7SEG_FONT[digit];
}
```

The hex values are bit-for-bit the original `B*` constants (`B11000000`==`0xC0`, etc.), so every
migrated driver produces the same bytes on the wire as before.

Host tests (`led7seg_font/test_led7seg_font.c`): assert each of 0-9 maps to its exact byte
(0xC0/0xCF/0xA4/0xB0/0x99/0x92/0x82/0xF8/0x80/0x90); assert out-of-range (10 and 255) → 0xFF.

## Migration (all 3 variants)

For each of `led7segShiftout/led7seg.c`, `led7segSpi/led7seg.c`, `led7segSpi(noISR)/led7seg.c`:
- Add `#include "led7seg_font.h"`.
- Delete the local `const uint8_t Led7seg[10] = { ... };`.
- Replace every `Led7seg[value]` / `Led7seg[value % 10]` with `led7seg_segments(value)` /
  `led7seg_segments(value % 10)`.
- Leave everything else (decompose loops, `common` shifting, modes, `binary.h` display
  constants, SPI/shiftOut calls) untouched. Net behavior identical.

## Test harness (extend avr-libs Makefile)

- Add `led7seg` host-test target: `cc … -Iled7seg_font led7seg_font/test_led7seg_font.c led7seg_font/led7seg_font.c third_party/unity/unity.c` + run. Fold into `test`.
- `strict`: add `led7seg_font/led7seg_font.c` with `-Werror`.
- `avr`: add `led7seg_font/led7seg_font.c` (`$(AVR_CC) $(AVRFLAGS) -c …`).
- Best-effort `avr-gcc` compile of each migrated driver with `-Iled7seg_font` (+ each variant's own dir); compile-gate only (the display path is not host-testable). Document if the vendored include soup blocks any.

## Conventions
- `CLIBS_LED7SEG_FONT_H` guard; freestanding (`<stdint.h>` only); doc comment on the public function.
- Standard hex literals (no `binary.h` dependency in the shared unit).

## Build sequence
1. `led7seg_font` unit + Makefile `led7seg` target → test (RED) → implement → GREEN.
2. Migrate `led7segShiftout/led7seg.c` → best-effort AVR compile.
3. Migrate `led7segSpi/led7seg.c` → best-effort AVR compile.
4. Migrate `led7segSpi(noISR)/led7seg.c` → best-effort AVR compile.
5. Final gates: `make test` (8 suites now), `make strict`, `make avr`; README note.
6. Feature branch in avr-libs → merge there → push avr-libs → bump gitlink in clibs.

(Tasks 2-4 can be one task — three mechanical, identical migrations verified by the same compile-gate.)
