# avr-libs cycle 2a — pure-logic modules (design)

Date: 2026-06-20
Repo: work happens inside the **`avr-libs` git submodule** (`mannk98/avr-libs`, branch `main`).
Planning docs live in the parent `clibs` repo (this file).

## Context

avr-libs is 16 register-level ATmega328p modules. They split by testability:
- **Pure-logic** (host-unit-testable) — this cycle: `myPid`, `goot80_typeK`, `t33Soldering`, `ringBuffer`.
- **Register drivers** (need simulator/hardware) — later cycles: adc, spi, timer, uartRingbuff, led7seg*, max6675, RTOS.

Testing strategy (user-chosen): **Hybrid** — host-unit-test the pure logic with Unity + an `avr-gcc` cross-compile gate; drivers refactored/reviewed later. avr-gcc 9.5.0 is installed via brew, **keg-only** at `/opt/homebrew/opt/avr-gcc@9/bin/avr-gcc` (not on PATH).

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Cycle 2a scope | `myPid`, `goot80_typeK`, `t33Soldering`, `ringBuffer` only |
| Test framework | Unity, vendored **separately** into `avr-libs/third_party/unity/` (standalone repo) |
| Compile gate | `avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL` per module |
| `volToTemp` collision | Rename to unique `goot80_vol_to_temp` / `t33_vol_to_temp` + unique guards |
| `ringBuffer` | Full rewrite → malloc-free byte FIFO over caller storage (`ringbuf_*` API), replacing `ringBuff_*` |
| Calibration | Preserve the temp-curve formulas byte-for-byte (characterization tests, no hardware to verify °C) |
| Git workflow | Feature branch in the avr-libs submodule → merge there → push avr-libs → bump gitlink in clibs |

## Test harness (in avr-libs)

- Vendor Unity into `avr-libs/third_party/unity/` (`unity.c`, `unity.h`, `unity_internals.h`, `LICENSE.txt`) — its own copy; avr-libs is standalone and cannot reference clibs/common.
- `avr-libs/Makefile`:
  - `make test` — host-run Unity suites (`test_pid`, `test_goot80`, `test_t33`, `test_ringbuf`).
  - `make avr` — cross-compile gate: `$(AVR_CC) -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -c <module>.c -o /dev/null` for each module. `AVR_CC ?= /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc` (overridable; falls back to `avr-gcc` on PATH).
  - `make strict` — host `-std=c11 -Wall -Wextra -Werror` compile of the owned modules.
  - `make clean`.
- `avr-libs/.gitignore` adds `build/`.

## Module specs

### `myPid` — minimal change + tests
Already pure float math, host-compilable, no hardware deps. Keep the API and `volatile PIDController*` (ISR-friendly). No rewrite. Add Unity characterization tests:
- P-only (Ki=Kd=0): `out == Kp*error`, clamped to limMax/limMin.
- Integrator: trapezoidal accumulation; anti-windup clamp at limMaxInt/limMinInt.
- Derivative-on-measurement sign (rising measurement → negative derivative contribution).
- Output clamp to limMax/limMin.
- Convergence: constant setpoint over N updates drives `out` toward a stable value (monotonic error reduction for a simple plant model in the test).
File names kept (`PID.c`/`PID.h`, guard `PID_H_`) to minimize churn.

### `goot80_typeK` & `t33Soldering` — testable + de-collided
- Replace `#include <avr/io.h>` with `#include <stdint.h>`.
- Delete the large dead commented-out `nthRoot`/`my_pow`/`findNthRoot` blocks.
- Rename: `float goot80_vol_to_temp(uint16_t adc_value)` in guard `CLIBS_GOOT80_TYPEK_H`; `float t33_vol_to_temp(uint16_t adc_value)` in guard `CLIBS_T33_SOLDERING_H`.
- Keep the formulas EXACTLY:
  - goot80: `v = adc*0.02237`; `adc==0 → 0`; bands at 7.739/10.971/14.293 with slopes 24.95/24.7/24.1/23.8 (intercepts −3.12/−0.97/+9.67/+44.68; offsets all 0).
  - t33: `v = adc*0.00716`; `adc==0 → 0`; bands at 2.919/4.321/5.789 with slopes 62.98/57.07/54.47/52.77 (intercepts +7.74/+23.74/+34.88/+44.68; offsets +25/+32/+23/+19).
- Characterization tests: assert each function's output equals the formula (computed in-test) at sample ADC values in every band + boundaries + `adc==0 → 0`, using `TEST_ASSERT_FLOAT_WITHIN`.

### `ringBuffer` — full rewrite (malloc-free byte FIFO)
Current defects: `#define uint8_t char` (redefines a stdlib type); `malloc` in init; fake `isBusy` lock; **busy-wait-forever on full** (`while(ringBuffFull(self));`); `ringBuff_getBytes` wrong success condition (`count == len-1`); `putString` compares `*string != NULL` instead of `'\0'`. Rewrite to caller-provided storage, byte-specialized, clean:
```c
typedef enum { RB_OK, RB_FULL, RB_EMPTY, RB_OVERWRITE, RB_INVALID } rb_err;
typedef struct { uint8_t *buf; uint16_t cap, head, count; bool overwrite; } ringbuf;

void     ringbuf_init(ringbuf *self, uint8_t *storage, uint16_t cap, bool overwrite);
rb_err   ringbuf_put(ringbuf *self, uint8_t byte);     /* full: RB_OVERWRITE (evict) or RB_FULL */
rb_err   ringbuf_get(ringbuf *self, uint8_t *out);     /* out may be NULL to drop; RB_EMPTY if none */
rb_err   ringbuf_peek(const ringbuf *self, uint8_t *out);
rb_err   ringbuf_put_string(ringbuf *self, const char *s);          /* '\0'-terminated; per-byte put */
uint16_t ringbuf_get_bytes(ringbuf *self, uint8_t *out, uint16_t max); /* returns #bytes copied */
uint16_t ringbuf_count(const ringbuf *self);
bool     ringbuf_is_empty(const ringbuf *self);
bool     ringbuf_is_full(const ringbuf *self);
void     ringbuf_clear(ringbuf *self);
```
- Header guard `CLIBS_RINGBUFF_H`; includes only `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`.
- Index math widened to avoid 16-bit overflow (as in common/ringbuf).
- No fake lock; document "not reentrant; guard with ATOMIC_BLOCK/cli if shared with an ISR".
- `ringbuf_init` guards NULL storage / zero cap → safe invalid state.
- `ringbuf_put_string` stops at `'\0'`; returns RB_OVERWRITE if any byte evicted, else RB_OK / RB_FULL.
- `ringbuf_get_bytes` copies up to `max` (or until empty), returns the count actually copied (fixes the old bug).
- Full Unity tests incl. regressions: FIFO order, overwrite-evict, reject-when-full, wraparound, peek, put_string, get_bytes count correctness, NULL-arg guards, and explicitly that there is NO infinite loop on full.
- The 4 driver-vendored `ringbuff` copies (in spi/, uartRingbuff/, led7segSpi/, led7segSpi(noISR)/) are NOT touched this cycle.

## Conventions
- `CLIBS_*_H` guards, prefixed enums, doc comments on public functions.
- No fake thread-safety; honest ISR-guard contract.
- Owned modules host-compile freestanding AND avr-cross-compile clean.

## Bugs fixed (each regression-tested)
1. `ringbuff.h` `#define uint8_t char`.
2. `ringBuff_getBytes` wrong success condition.
3. `ringBuff_put` infinite busy-wait when full.
4. `ringBuff_init` heap allocation (malloc) — now static caller storage.
5. fake `isBusy` cooperative lock.
6. `putString` `*string != NULL` (should be `'\0'`).
7. `typeK.h`/`T33Soldering.h` `<avr/io.h>` breaks host build.
8. `volToTemp` + `SOLDERING_H_` name/guard collision between the two soldering modules.

## Out of scope (this cycle)
- All register drivers, RTOS, max6675 (incl. its `readFah`→`readCelsius()` bug — driver cycle).
- The 4 driver-vendored `ringbuff` copies (migrate in the driver cycle).
- Eclipse `.project`/`.settings`/`.metadata` (leave as-is).
- Cross-repo unification of avr-libs ringbuf with clibs/common/ringbuf (deliberately kept separate; vendored).

## Build sequence
1. Scaffold: vendor Unity + `Makefile` + `.gitignore` in avr-libs; feature branch.
2. `myPid` — tests (module already clean) → green.
3. `goot80_typeK` — refactor (stdint, rename, dead-code) → tests → green.
4. `t33Soldering` — same → green.
5. `ringBuffer` — rewrite → tests → green.
6. Final gates: `make test` (4 suites), `make strict`, `make avr`; update avr-libs README module notes if needed.
7. Merge feature branch in avr-libs, push avr-libs, bump + commit the gitlink in clibs.
