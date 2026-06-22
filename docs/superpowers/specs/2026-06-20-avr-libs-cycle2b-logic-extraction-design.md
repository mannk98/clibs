# avr-libs cycle 2b — driver logic extraction (design)

Date: 2026-06-20
Repo: work happens inside the **`avr-libs` git submodule** (`mannk98/avr-libs`, branch `main`).
Planning docs live in the parent `clibs` repo (this file).

## Context

Cycle 2a host-tested the 4 standalone pure-logic modules. This cycle (2b) extracts the
**pure algorithmic core** of three register drivers into hardware-free sub-units that can be
host-unit-tested with Unity, leaving the existing driver as a thin wrapper that calls the
sub-unit. No physical hardware and no simulator needed.

Testing strategy: same hybrid as 2a — host Unity for the pure sub-units + `avr-gcc`
cross-compile gate. avr-gcc 9.5 is at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc` (keg-only).

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope cycle 2b | `max6675`, `encoder`, `wallClock` only (skip `adc` — averaging too trivial; `led7seg` deferred) |
| Seam pattern | A hardware-free **pure sub-unit** (`_conv`/`_decode`/`_tick`) per module; the driver becomes a thin wrapper that calls it (DRY — one source of the logic) |
| max6675 bug | Fix `readFah` (called undefined `readCelsius()`); drop the bad `#define NAN -100.0` |
| AVR gate | Covers the **pure sub-units**. Full driver AVR-compile is deferred to the dedup cycle (drivers pull the vendored `wiring_digital.h`/pin-mapping include soup) — driver wiring edits verified by review + best-effort compile |
| Out of scope | `adc`, `led7seg`, RTOS, spi, the vendored-copy dedup, full driver compilation |

## Module specs

### `max6675` → new `max6675_conv.{c,h}` (hardware-free)
```c
/* max6675_conv.h — guard CLIBS_MAX6675_CONV_H, includes <stdint.h> only */
#define MAX6675_OPEN (-100.0f)                  /* returned when thermocouple open (raw bit2 set) */
float max6675_raw_to_celsius(uint16_t raw);     /* (raw & 0x4) ? MAX6675_OPEN : (raw >> 3) * 0.25f */
float max6675_celsius_to_fahrenheit(float c);   /* c * 9.0f / 5.0f + 32.0f */
```
- Behavior preserved from the original `readCel`: open-thermocouple sentinel was `NAN` = `-100.0`; now `MAX6675_OPEN` = `-100.0f` (same value, same meaning).
- `max6675.c` wiring: `readCel` returns `max6675_raw_to_celsius(v)` (after the SPI read builds `v`); **`readFah` returns `max6675_celsius_to_fahrenheit(readCel(self))`** — fixes the undefined-`readCelsius()` bug. Remove the `#define NAN -100.0` from `max6675.h` (no longer referenced).
- Tests (`test_max6675_conv.c`): open bit set (e.g. `0x0004`, `0x0C84`) → `MAX6675_OPEN`; `raw=3200` (`0x0C80`, bit2 clear) → `100.0` °C; `raw=8` → `0.25`; `celsius_to_fahrenheit`: 0→32, 100→212, -40→-40.

### `encoder` → new `encoder_decode.{c,h}` (hardware-free)
```c
/* encoder_decode.h — guard CLIBS_ENCODER_DECODE_H, includes <stdint.h> only */
/* One poll: a/b are the just-read pin states. Advances the detent state (*pre_a,*count);
 * on every 2nd A-edge (a detent), applies +1 (a==b) or -1 (a!=b) to *counter. */
void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter);
```
Logic preserved byte-for-byte from `encoderCheck`'s decode:
```c
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
- `encoder.c` wiring: `encoderCheck` reads `pinA_State`/`pinB_State` via `digitalRead`, then calls `encoder_decode(&self->pre_pinA_State, &self->count, pinA_State, pinB_State, (int *)counter)`.
- Tests (`test_encoder_decode.c`): CW detent — feed `(a=1,b=1)` then `(a=0,b=0)` → `counter == +1`; CCW — `(1,0)` then `(0,1)` → `counter == -1`; A unchanged → no count/counter change; one edge only (count==1) → counter unchanged until the 2nd edge.

### `wallClock` → new `wallclock_tick.{c,h}` (hardware-free, needs `F_CPU`)
```c
/* wallclock_tick.h — guard CLIBS_WALLCLOCK_TICK_H, includes <stdint.h> only */
typedef struct { uint32_t millis; uint8_t fract; } wallclock_accum;
void wallclock_tick(wallclock_accum *a);   /* one Timer0 overflow */
```
`wallclock_tick.c` defines the F_CPU-derived constants (verbatim from `wallCLock.c`) and the accumulate:
```c
#define MICROSECONDS_PER_TIMER0_OVERFLOW (64 * 256 / (F_CPU / 1000000L))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)        /* 16MHz -> 1 */
#define FRACT_INC  ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3) /* 16MHz -> 24>>3 = 3 (NOT 6) */
#define FRACT_MAX  (1000 >> 3)                                      /* 125 */

void wallclock_tick(wallclock_accum *a) {
    a->millis += MILLIS_INC;
    a->fract  += FRACT_INC;
    if (a->fract >= FRACT_MAX) { a->fract -= FRACT_MAX; a->millis += 1; }
}
```
- Compiled with `-DF_CPU=16000000UL` (host + avr). Corrects the stale `wallCLock.c` comment claiming `FRACT_INC = 24>>3 = 6` (it is 3).
- `wallCLock.c` wiring: the `TIMER0_OVF_vect` ISR loads `timer0_millis`/`timer0_fract` into a `wallclock_accum`, calls `wallclock_tick`, writes back, then `timer0_overflow_count++`. `millis()`/`micros()`/register setup untouched.
- Tests (`test_wallclock_tick.c`, `-DF_CPU=16000000UL`): 1 tick → `millis==1, fract==3`; 1000 ticks → `millis==1024, fract==0` (the 1.024 ms/overflow correction); accumulation monotonic.

## Test harness (extend existing avr-libs Makefile)

Add three host-test targets and fold them into `test`, `strict`, `avr`:
- `make max6675` → `cc … -Imax6675 -o build/test_max6675 max6675/test_max6675_conv.c max6675/max6675_conv.c third_party/unity/unity.c && run`
- `make encoder` → likewise with `encoder/encoder_decode.c` + `encoder/test_encoder_decode.c`
- `make wallclock` → likewise **plus `-DF_CPU=16000000UL`**, `wallClock/wallclock_tick.c` + `wallClock/test_wallclock_tick.c`
- `strict`: add the 3 sub-units with `-Werror` (wallclock_tick with `-DF_CPU=16000000UL`).
- `avr`: add the 3 sub-units (`$(AVR_CC) $(AVRFLAGS) -c …`; wallclock_tick with `-DF_CPU` already in AVRFLAGS).

The sub-units are hardware-free → host-compile clean. The AVR gate proves they build for atmega328p.

## Driver wiring + scope boundary
The driver edits (`max6675.c`, `encoder.c`, `wallCLock.c`) are minimal — replace inline logic with a call to the sub-unit (+ the readFah fix). They `#include` the vendored `wiring_digital.h`/pin-mapping, so they don't compile standalone; **fully AVR-compiling the drivers is the dedup cycle's job.** This cycle verifies the driver edits by review + a best-effort `avr-gcc` compile with `-I` flags (documented if the include soup blocks it). No behavior change beyond the readFah bug fix.

## Conventions
- `CLIBS_*_H` guards on the new sub-units; freestanding (`<stdint.h>` only; wallclock_tick additionally needs `F_CPU` defined at compile time).
- Doc comments on every public function; conversion/decoding math preserved exactly (only readFah fixed).

## Bugs fixed (regression-tested)
1. `max6675.c` `readFah` calls undefined `readCelsius()` → now `max6675_celsius_to_fahrenheit(readCel(self))`.
2. `wallCLock.c` stale comment `FRACT_INC = 24>>3 = 6` → corrected to 3 (verified by the 1000-tick test).

## Build sequence
1. `max6675_conv` — test → implement → green; wire `max6675.c` (readCel + readFah fix, drop NAN macro).
2. `encoder_decode` — test → implement → green; wire `encoder.c`.
3. `wallclock_tick` — test → implement → green; wire `wallCLock.c` ISR.
4. Final gates: `make test` (pid 5, goot80 6, t33 6, ringbuf 12, max6675 N, encoder N, wallclock N), `make strict`, `make avr`; best-effort driver AVR compile; README note.
5. Feature branch in avr-libs → merge there → push avr-libs → bump gitlink in clibs.
