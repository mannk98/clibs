# esp-libs cycle 8 — Layer-3 drivers: `ds18b20` + `servo` + `hcsr04` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project).

## Context

`clibs` is **libraries only**. Per the pre-hardware roadmap (A L1-libs → D CI → C bus-wrappers → **B more L3**),
this cycle adds three more L3 device drivers, each pairing a host-tested pure part with hardware-deferred glue:

- **`ds18b20`** — 1-Wire temperature sensor; reuses the cycle-6 `crc8_maxim` for the scratchpad CRC.
- **`servo`** — servo/ESC via software PWM (angle → pulse width).
- **`hcsr04`** — ultrasonic distance sensor (echo time → mm).

Same hybrid posture as cycles 4/5: the **pure logic** (`ds18b20_parse_temp`, `servo_angle_to_duty`,
`hcsr04_us_to_mm`) is **host-Unity-tested** in SDK-header-free files; the **bit-bang/PWM glue** is
**compile-gate only**, with **runtime correctness hardware-deferred** (1-Wire/echo timing, PWM output need a
real chip). `make test` grows 14 → 17 suites.

**Build env (Step-0 recipe; legacy `make`):**
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig && make
```

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | `ds18b20` + `servo` + `hcsr04`. `bme280` (I2C, Bosch compensation) is its own later cycle. |
| Test | host-Unity for the three pure parts; compile-gate for the glue; hardware run out of scope. |
| ds18b20 | reuse cycle-6 `crc8_maxim` for the scratchpad CRC; output milli-°C (int32). |
| servo | `servo_angle_to_duty(angle 0..180)` → pulse 1000..2000 µs; software PWM at 50 Hz (period 20000 µs). |
| hcsr04 | `hcsr04_us_to_mm(echo_us)` = `echo_us * 343 / 2000` (speed of sound, round-trip /2). |
| Out of scope | `bme280`, `ota`, real-hardware runtime verification. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: +3 host-test targets (test -> 17 suites) + strict for the 3 pure .c
├── component.mk    # CHANGED: +3 dirs in all 3 lists + 6 .o (pure+glue per driver)
├── <existing dirs> # unchanged
├── ds18b20/   ds18b20_parse.{h,c}  ds18b20.{h,c}  test_ds18b20_parse.c
├── servo/     servo_duty.{h,c}     servo.{h,c}    test_servo_duty.c
└── hcsr04/    hcsr04_dist.{h,c}    hcsr04.{h,c}   test_hcsr04_dist.c
clibs/esp-gate/main/main.c   # CHANGED: touch the 3 drivers
```

Split per driver (cycle-4/5 convention): a **pure `.c`** (no SDK header → host-compilable + Unity-tested) +
a **glue `.c`** (SDK headers → compile-gate). `ds18b20_parse.c` includes the cycle-6 `crc.h` (pure). No malloc.

## Module specs

### `ds18b20` (`esp-libs/ds18b20/`)
Pure `ds18b20_parse.h` (guard `CLIBS_DS18B20_PARSE_H`, `<stdint.h>`,`<stdbool.h>`):
```c
/* Validate the 9-byte scratchpad (crc8_maxim(scratch,8) == scratch[8]) and convert the
 * temperature (scratch[0]=LSB, scratch[1]=MSB; LSB = 1/16 C) to milli-degrees C.
 * Bad CRC / NULL -> false. */
bool ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc);
```
- Impl: NULL guards; `if (crc8_maxim(scratch, 8) != scratch[8]) return false;`
  `int16_t raw = (int16_t)((uint16_t) scratch[1] << 8 | scratch[0]); *temp_mc = (int32_t) raw * 625 / 10; return true;`
  (`ds18b20_parse.c` includes `crc.h`.)
- Tests: a valid frame for +85.000 °C (raw 0x0550=1360 → 85000 mC), CRC byte = `crc8_maxim(scratch,8)`;
  a −10.000 °C frame (raw 0xFF60 = −160 → −10000 mC); a bad-CRC frame (flip scratch[0], keep CRC) → false;
  NULL guards.

Glue `ds18b20.h` (guard `CLIBS_DS18B20_H`, `esp_err.h`, `driver/gpio.h`, `ds18b20_parse.h`):
```c
typedef struct { gpio_num_t pin; } ds18b20;
esp_err_t ds18b20_init(ds18b20 *self, gpio_num_t pin);
/* 1-Wire single-drop read; fills milli-degC. ESP_OK / ESP_ERR_TIMEOUT / ESP_ERR_INVALID_CRC. */
esp_err_t ds18b20_read(ds18b20 *self, int32_t *temp_mc);
```
- `ds18b20.c` (bit-bang via `rom/ets_sys.h` `ets_delay_us` + `driver/gpio.h`): 1-Wire reset+presence; skip-ROM
  (0xCC) + convert-T (0x44); wait; reset; skip-ROM + read-scratchpad (0xBE); read 9 bytes; `ds18b20_parse_temp`
  → `ESP_ERR_INVALID_CRC` on parse failure. Bounded waits → `ESP_ERR_TIMEOUT`. **Timing is a first cut to tune
  on hardware.** A standard 1-Wire reset/read-bit/write-bit helper trio guards each level edge with a timeout.

### `servo` (`esp-libs/servo/`)
Pure `servo_duty.h` (guard `CLIBS_SERVO_DUTY_H`, `<stdint.h>`):
```c
/* Map angle (clamped to 0..180) to a servo pulse width in microseconds (1000..2000). */
uint32_t servo_angle_to_duty(uint8_t angle);
```
- Impl: `if (angle > 180) angle = 180; return 1000u + (uint32_t) angle * 1000u / 180u;`
- Tests: 0→1000, 90→1500, 180→2000, 200→2000 (clamp), 45→1250.

Glue `servo.h` (guard `CLIBS_SERVO_H`, `esp_err.h`, `driver/gpio.h`, `servo_duty.h`):
```c
typedef struct { uint8_t angle; bool started; } servo;
esp_err_t servo_init(servo *self, gpio_num_t pin);  /* software PWM, 50 Hz (period 20000 us) */
esp_err_t servo_set(servo *self, uint8_t angle);
```
- `servo.c` (`driver/pwm.h`): `pwm_init(20000, duties{servo_angle_to_duty(90)}, 1, pins{pin})` + `pwm_start`;
  `servo_set`: `pwm_set_duty(0, servo_angle_to_duty(angle))` + `pwm_start`; store angle. NULL/`!started` guards.
  **Note:** the SDK PWM is one global subsystem — `servo` and `pwm_dimmer` share channel 0, so only one runs at
  a time (documented).

### `hcsr04` (`esp-libs/hcsr04/`)
Pure `hcsr04_dist.h` (guard `CLIBS_HCSR04_DIST_H`, `<stdint.h>`):
```c
/* Convert an echo-high duration (us) to distance in mm: us * 343 / 2000 (speed of sound,
 * round-trip halved). Valid echo <= ~38000 us (the uint32 product does not overflow). */
uint32_t hcsr04_us_to_mm(uint32_t echo_us);
```
- Impl: `return echo_us * 343u / 2000u;`
- Tests: 0→0, 5800→994, 58→9 (≈1 cm), 38000→6517.

Glue `hcsr04.h` (guard `CLIBS_HCSR04_H`, `esp_err.h`, `driver/gpio.h`, `hcsr04_dist.h`):
```c
typedef struct { gpio_num_t trig; gpio_num_t echo; } hcsr04;
esp_err_t hcsr04_init(hcsr04 *self, gpio_num_t trig, gpio_num_t echo);
/* Trigger + measure echo; fills mm. ESP_OK / ESP_ERR_TIMEOUT (no echo within bound). */
esp_err_t hcsr04_read(hcsr04 *self, uint32_t *mm);
```
- `hcsr04.c` (`driver/gpio.h` + `rom/ets_sys.h`): drive trig high 10 µs then low; bounded-wait for echo rising
  then measure the high duration (us counter + `ets_delay_us(1)`); `*mm = hcsr04_us_to_mm(dur)`. No edge →
  `ESP_ERR_TIMEOUT`. **Timing is a first cut to tune on hardware.**

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `ds18b20_parse crc`-linked target + `servo_duty` + `hcsr04_dist` to `.PHONY` and
  `test:` (→ 17 suites); each compiles its `test_*.c` + the pure `.c` (ds18b20 also links `crc/crc.c`) + Unity.
  `strict:` adds `-Werror` compiles of the 3 pure `.c` (ds18b20_parse with `-Icrc`).
- **`esp-libs/component.mk`:** add `ds18b20 servo hcsr04` to both dir lists; add `ds18b20/ds18b20_parse.o
  ds18b20/ds18b20.o servo/servo_duty.o servo/servo.o hcsr04/hcsr04_dist.o hcsr04/hcsr04.o` to `COMPONENT_OBJS`.
  `test_*.c` excluded; `COMPONENT_REQUIRES` unchanged.
- **`esp-gate/main/main.c`:** include the 3 glue headers + a touch block calling one+ symbol of each.

## Testing
- **Host:** `make test` → **17 suites** (14 + ds18b20_parse, servo_duty, hcsr04_dist), all `0 Failures`;
  `make strict` → `strict: OK`.
- **ESP compile-gate:** `cd esp-gate && make` → links `build/esp_libs_gate.elf`; the 3 glue symbols
  (`ds18b20_read`, `servo_set`, `hcsr04_read`) present as `T`.
- **Hardware: out of scope** — 1-Wire / echo timing + PWM output verified on a real chip later.

## Conventions & scope
- Pure `.c` SDK-header-free (host-compilable); glue `.c` includes SDK headers; SDK decls in the glue header.
- `CLIBS_*_H` guards; `esp_err_t` for glue, `int32_t`/`uint32_t`/`bool` for pure; NULL/edge guards; no malloc;
  `ds18b20_parse` reuses cycle-6 `crc8_maxim` (no duplicated CRC).
- Out of scope: `bme280`, `ota`, real-hardware verification.

## Build sequence (independent drivers → parallel dev, serial integrate)
1. `ds18b20` — pure parse (+host test, RED→GREEN, reuses crc) + 1-Wire glue.
2. `servo` — pure angle→duty (+host test) + PWM glue.
3. `hcsr04` — pure us→mm (+host test) + echo glue.
4. Integration — wire all 3 into `Makefile`/`component.mk`/`esp-gate`; `make test` (17) + `strict` + esp-gate link;
   README L3 section.
5. (clibs feature branch → merge → push.)

Each driver's pure part is independent + host-testable, so 1–3 can be developed in parallel; the integration
step wires + runs the full gate.
