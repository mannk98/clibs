# esp-libs cycle 5 — Layer-3 device drivers: `relay` + `button` + `dht` + `pwm_dimmer` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project).

## Context

`clibs` is **libraries only** (the node app is a separate repo). Layer 1 (pure logic) and Layer 2
(SDK wrappers: wifi_sta, mqtt_node, nvs_kv, device_id, json, periodic, mdns_node) are done. This
cycle starts **Layer 3 — device drivers**: the per-peripheral code a node uses to drive actuators
and read sensors. Four drivers, one per node entity type:

- **`relay`** — GPIO output actuator (on/off/toggle, active-low support).
- **`button`** — GPIO input + the L1 `debounce` FSM (edge events).
- **`dht`** — DHT22/DHT11 temperature + humidity (bit-banged 1-wire-style frame).
- **`pwm_dimmer`** — brightness/speed via the SDK software PWM.

**Test posture (user-confirmed): code now, hardware-test later.** Same hybrid as L1/L2 plus a
hardware caveat: the **pure logic** of each driver (`relay_level`, `dht_parse`, `dimmer_duty`) is
**host-Unity-tested**; the **GPIO/PWM/timing glue** is **compile-gate only**; **running on real
hardware is out of scope** — when hardware arrives the user will run one pass and fix (especially
`dht`'s bit-bang timing, which is the most likely to need tuning).

**Build env (Step-0 recipe; legacy `make`, NOT idf.py):**
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig && make   # links build/esp_libs_gate.elf
```
SDK includes: `driver/gpio.h` (gpio_config / gpio_set_level / gpio_get_level / gpio_set_direction),
`driver/pwm.h` (pwm_init / pwm_set_duty / pwm_start), `rom/ets_sys.h` (`ets_delay_us`). All live in
the always-present `esp8266` component.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | `relay` + `button` + `dht` + `pwm_dimmer` (all four). `ds18b20` (1-Wire) is a later cycle. |
| Test | host-Unity for the pure parts; compile-gate for GPIO/PWM/timing glue; **no hardware run this cycle**. |
| relay | active-low support; on/off/toggle/is_on. |
| button | reuses the L1 `debounce` FSM; poll-based (caller samples at a fixed interval). |
| dht | pure `dht_parse` (5-byte frame → deci-°C / deci-%RH) host-tested; bit-bang read compile-gated (tune on hardware). |
| pwm_dimmer | pure `dimmer_duty` (percent→duty) host-tested; single global PWM channel (one dimmer) — documented. |
| Out of scope | `ds18b20`, OTA, `node_core` app, hardware/runtime verification. |

## Architecture

```
clibs/esp-libs/
├── Makefile            # CHANGED: +relay_level +dht_parse +dimmer_duty host targets (test -> 9 suites)
├── component.mk        # CHANGED: +4 dirs in all 3 lists
├── <L1 + L2 dirs>      # unchanged
├── relay/       relay_level.{h,c}  relay.{h,c}  test_relay_level.c
├── button/      button.{h,c}
├── dht/         dht_parse.{h,c}  dht.{h,c}  test_dht_parse.c
└── pwm_dimmer/  dimmer_duty.{h,c}  pwm_dimmer.{h,c}  test_dimmer_duty.c
clibs/esp-gate/main/main.c   # CHANGED: touch the 4 drivers' symbols
```

Same split rule as cycle 4: a **pure `.c`** (no SDK header — host-compilable) holds the testable
logic; a **separate SDK-glue `.c`** holds the GPIO/PWM/timing code. `button` has no host-testable
pure part of its own (its logic is the L1 `debounce`), so it is a single compile-gate-only file.

## `relay` (`esp-libs/relay/`)

Pure header `relay_level.h` (guard `CLIBS_RELAY_LEVEL_H`, `<stdbool.h>`):
```c
/* GPIO level (0/1) to drive for the desired on-state; active_low inverts. */
int relay_level(bool on, bool active_low);
```
SDK header `relay.h` (guard `CLIBS_RELAY_H`, `<stdbool.h>`, `esp_err.h`, `driver/gpio.h`, `relay_level.h`):
```c
typedef struct { gpio_num_t pin; bool active_low; bool on; } relay;
esp_err_t relay_init(relay *self, gpio_num_t pin, bool active_low); /* output mode; driven OFF */
esp_err_t relay_set(relay *self, bool on);                          /* gpio_set_level(pin, relay_level(on,active_low)) */
esp_err_t relay_toggle(relay *self);
bool      relay_is_on(const relay *self);
```
- `relay_init`: NULL guard; `gpio_set_direction(pin, GPIO_MODE_OUTPUT)` (or `gpio_config`), then `relay_set(self,false)`.
- `relay_set`: NULL guard; `gpio_set_level(pin, (uint32_t)relay_level(on, active_low))`; store `on`.
- `relay_toggle`: `relay_set(self, !self->on)`. `relay_is_on`: returns `self->on` (false if NULL).
- Tests (`test_relay_level.c`): `relay_level(true,false)`=1, `(false,false)`=0, `(true,true)`=0, `(false,true)`=1.

## `button` (`esp-libs/button/`)  — compile-gate only

SDK header `button.h` (guard `CLIBS_BUTTON_H`, `<stdbool.h>`,`<stdint.h>`, `esp_err.h`, `driver/gpio.h`, `debounce.h`):
```c
typedef enum { BUTTON_NONE, BUTTON_PRESSED, BUTTON_RELEASED } button_event;
typedef struct { gpio_num_t pin; bool active_low; debounce db; } button;
esp_err_t    button_init(button *self, gpio_num_t pin, bool active_low, uint8_t threshold);
button_event button_poll(button *self);   /* call at a fixed interval */
bool         button_is_pressed(const button *self);
```
- `button_init`: NULL guard; `gpio_set_direction(pin, GPIO_MODE_INPUT)` + `gpio_set_pull_mode` (pull-up if active_low, else pull-down); `debounce_init(&self->db, threshold ? threshold : 1, 0)`.
- `button_poll`: read `gpio_get_level(pin)`; `pressed_raw = active_low ? !level : level`; `edge = debounce_update(&self->db, pressed_raw)`; map `DEBOUNCE_RISING → BUTTON_PRESSED`, `DEBOUNCE_FALLING → BUTTON_RELEASED`, else `BUTTON_NONE`.
- `button_is_pressed`: `debounce_level(&self->db)` (false if NULL).
- No host test: the debounce FSM is already L1-tested; this driver is GPIO glue. Compile-gate only.

## `dht` (`esp-libs/dht/`)

Pure header `dht_parse.h` (guard `CLIBS_DHT_PARSE_H`, `<stdint.h>`,`<stdbool.h>`):
```c
/* Parse a 5-byte DHT frame. Returns true + fills outputs on valid checksum.
 * humidity = (b0<<8|b1) deci-%RH; temperature = ((b2&0x7F)<<8|b3) deci-°C, negative if b2&0x80.
 * checksum: (b0+b1+b2+b3) & 0xFF == b4. NULL arg / bad checksum -> false. */
bool dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct);
```
SDK header `dht.h` (guard `CLIBS_DHT_H`, `<stdint.h>`, `esp_err.h`, `driver/gpio.h`, `dht_parse.h`):
```c
typedef struct { gpio_num_t pin; } dht;
esp_err_t dht_init(dht *self, gpio_num_t pin);
/* Bit-bang a read; on success fills deci-units. ESP_OK / ESP_ERR_TIMEOUT / ESP_ERR_INVALID_CRC. */
esp_err_t dht_read(dht *self, int16_t *temp_dc, int16_t *humi_dpct);
```
- `dht.c`: standard DHT sequence — drive low ~1–18 ms, release, wait for the sensor's response
  (~80 µs low + 80 µs high), then read 40 bits (each bit = ~50 µs low then a high pulse whose length
  decides 0 vs 1), using `ets_delay_us` + `gpio_get_level` with bounded spin-wait loops that return
  `ESP_ERR_TIMEOUT` if a level edge never arrives. Assemble 5 bytes → `dht_parse` → on bad checksum
  return `ESP_ERR_INVALID_CRC`. **This timing is the part most likely to need hardware tuning.**
- Tests (`test_dht_parse.c`): a valid DHT22 frame `{0x02,0x92,0x01,0x4A,0xDF}` → humidity 658
  (65.8 %), temperature 330 (33.0 °C); a negative-temp frame (b2 high bit set); a bad-checksum frame
  → false; NULL guards.

## `pwm_dimmer` (`esp-libs/pwm_dimmer/`)

Pure header `dimmer_duty.h` (guard `CLIBS_DIMMER_DUTY_H`, `<stdint.h>`):
```c
/* Map percent (clamped to 0..100) to a duty in [0, max_duty], rounded. */
uint32_t dimmer_duty(uint8_t percent, uint32_t max_duty);
```
SDK header `pwm_dimmer.h` (guard `CLIBS_PWM_DIMMER_H`, `<stdint.h>`,`<stdbool.h>`, `esp_err.h`, `driver/gpio.h`, `dimmer_duty.h`):
```c
typedef struct { uint32_t period; uint32_t max_duty; uint8_t percent; bool started; } pwm_dimmer;
esp_err_t pwm_dimmer_init(pwm_dimmer *self, gpio_num_t pin, uint32_t period_us);
esp_err_t pwm_dimmer_set(pwm_dimmer *self, uint8_t percent);
```
- `pwm_dimmer_init`: NULL guard; `period = max_duty = period_us`; build `uint32_t pins[1]={pin}`,
  `uint32_t duties[1]={0}`; `pwm_init(period_us, duties, 1, pins)`; `pwm_start()`; `started=true`.
- `pwm_dimmer_set`: NULL/`!started` guard; `pwm_set_duty(0, dimmer_duty(percent, self->max_duty))`;
  `pwm_start()` (re-applies); store `percent`.
- **Limitation (documented):** the SDK PWM is a single global subsystem; this wrapper assumes one
  dimmer on channel 0. Multiple dimmers would need a multi-channel design (out of scope).
- Tests (`test_dimmer_duty.c`): `(0,1000)`=0, `(50,1000)`=500, `(100,1000)`=1000, `(255,1000)`=1000
  (clamp), `(33,100)`=33 (rounded).

## Build infrastructure changes

### `esp-libs/component.mk`
Add the four dirs to all three lists; OBJS gains:
```
relay/relay_level.o relay/relay.o button/button.o \
dht/dht_parse.o dht/dht.o pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o
```
(`test_*.c` still excluded.) If the ESP build fails on unresolved PWM/GPIO symbols, add `pwm` (and
whatever the gate names) to `COMPONENT_REQUIRES` — gpio/pwm are in the core `esp8266` component, so
likely no change is needed; the gate is the check.

### `esp-libs/Makefile`
Add three host targets (`relay_level`, `dht_parse`, `dimmer_duty`) to `.PHONY`, the `test:`
aggregate (→ 9 suites), and the `strict:` gate (compile each pure `.c` with `-Werror`). The SDK-glue
`.c` files are NOT in the host Makefile.

### `esp-gate/main/main.c`
Add includes + symbol-touch blocks (gate-only) for `relay_init/set`, `button_init/poll`,
`dht_init/read`, `pwm_dimmer_init/set`.

## Testing
- **Host:** `cd esp-libs && make test` → **9 suites** (the 6 existing + relay_level, dht_parse,
  dimmer_duty), all `0 Failures`; `make strict` → `strict: OK`.
- **ESP compile-gate:** `cd esp-gate && make` → links `build/esp_libs_gate.elf`; `nm` shows the new
  symbols as `T`.
- **Hardware: out of scope.** Bit-bang timing (dht) and PWM output are verified on real hardware in
  a later pass; correctness of the glue is asserted only at compile/link time here.

## Conventions & scope
- Pure `.c` files include NO SDK headers (host-compilable); SDK decls/types in separate SDK-only headers.
- Guards `CLIBS_*_H`; `esp_err_t` returns for SDK glue, `int`/`uint32_t`/`bool` for pure helpers;
  NULL-arg guards; caller-provided transparent struct storage, no malloc.
- `button` reuses L1 `debounce`; no driver duplicates L1 logic.
- Out of scope: `ds18b20`, OTA, `node_core`, hardware run.

## Build sequence
1. **`relay`**: `relay_level` (pure, host-test RED→GREEN) + `relay` SDK glue; wire Makefile/component.mk/esp-gate; host `make test` (7 suites) + esp-gate links.
2. **`button`**: SDK glue over L1 `debounce`; wire component.mk/esp-gate (no host suite); esp-gate links; host still 7 suites.
3. **`dht`**: `dht_parse` (pure, host-test) + `dht` bit-bang glue; wire; host `make test` (8 suites) + esp-gate links.
4. **`pwm_dimmer`**: `dimmer_duty` (pure, host-test) + `pwm_dimmer` glue; wire; host `make test` (9 suites) + esp-gate links.
5. **Final**: `make test` (9 suites) + `make strict` + esp-gate link + symbol check; README (Layer-3 section).
6. (clibs feature branch → merge → push.)
