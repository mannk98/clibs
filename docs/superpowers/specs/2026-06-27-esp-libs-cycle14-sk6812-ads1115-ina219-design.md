# esp-libs cycle 14 — SK6812 + ADS1115 + INA219 Design

**Status:** approved (brainstorm 2026-06-27)

**Goal:** Add three independent L3 drivers to esp-libs — `sk6812` (RGBW addressable LED), `ads1115` (16-bit 4-channel I2C ADC), `ina219` (I2C current/power monitor) — each header-split into a host-Unity-tested pure part + SDK glue.

## Background

esp-libs has `ws2812` (RGB addressable LED, `ws2812_render` pure + bit-bang glue) and four I2C devices (`bme280`, `bh1750`, `sht3x`, `ds3231`). `i2c_bus` has both a register API (`read_reg`/`write_reg`) and raw API (`read`/`write`). This cycle adds the RGBW sibling of ws2812 plus two analog-front-end I2C devices that extend sensing beyond the single A0 (ADS1115 = 4 external ADC channels; INA219 = current/power).

**Naming convention (as of cycle 13):** pure part = `<lib>.{h,c}` (freestanding, the TU named in `test_<lib>.c`, no SDK header); SDK glue = `<lib>_dev.{h,c}` (includes `esp_err.h` + bus headers). **Units:** micro/milli per convention — ADS1115 → microvolts; INA219 → bus millivolts, shunt microvolts, current microamps, power microwatts.

## Architecture

Three independent L3 stories run in parallel under embedded-sprint (`deps: []`), each header-split: pure `<lib>.{h,c}` (host-Unity-tested) + SDK glue `<lib>_dev.{h,c}` (xtensa COMPILE_GATE **deferred** — `xtensa-lx106-elf-gcc` absent). No bus prerequisite this cycle: SK6812 is GPIO bit-bang (like ws2812); ADS1115/INA219 use the existing `i2c_bus` register API (16-bit big-endian registers). Integration wires `esp-libs/Makefile` (test 32 → **35**), `component.mk`, `esp-gate/main/main.c`, `README`.

### Conventions (per profile-clibs)

- Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable.
- Glue files include `esp_err.h` + `i2c_bus.h` (or `driver/gpio.h`/`rom/ets_sys.h` for sk6812 bit-bang); not host-runnable.
- `CLIBS_<MODULE>_H` guards; NULL-guard every public pointer param; doc comment per public fn; "Not reentrant".
- **No UB:** ADS1115/INA219 conversions widen to `int64` before the multiply (map_range "widen-before-multiply"); shifts only on positive uint8/uint16/uint32; no left-shift of a possibly-negative value.

## C14-1 `sk6812` — RGBW addressable LED (mirrors ws2812)

**Pure** `esp-libs/sk6812/sk6812.{h,c}`:
```c
/* One pixel as the caller supplies it (R, G, B, W). */
typedef struct { uint8_t r, g, b, w; } sk6812_rgbw;

/* Render n pixels into an SK6812 GRBW byte buffer: out[4i]=G, out[4i+1]=R,
 * out[4i+2]=B, out[4i+3]=W, each channel scaled by brightness/255. need = 4*n.
 * Returns need, or 0 if out_size < need / NULL px or out / n == 0. Pure. */
size_t sk6812_render(const sk6812_rgbw *px, size_t n, uint8_t brightness, uint8_t *out, size_t out_size);
```
- `scale(c,brightness) = (uint16_t)c * brightness / 255` (uint16 product ≤ 65025, no overflow/UB) — identical to ws2812's helper.
- Vectors: `{r10,g20,b30,w40}` n1 bright255 → `{20,10,30,40}`; bright128 → `{10,5,15,20}`; `out_size 3 < need 4` → 0; NULL px/out → 0; n 0 → 0.

**Glue** `sk6812/sk6812_dev.{h,c}` — mirror `ws2812`'s glue: a `sk6812` struct holding the GPIO pin + render buffer, an init, and a bit-bang "show" that clocks 32 bits/pixel (800 kHz, FIRST CUT). COMPILE_GATE deferred.

## C14-2 `ads1115` — 16-bit 4-channel I2C ADC (addr 0x48)

**Pure** `esp-libs/ads1115/ads1115.{h,c}`:
```c
/* Programmable-gain full-scale range (config PGA field [11:9] = 0..5). */
typedef enum {
    ADS1115_PGA_6V144 = 0, ADS1115_PGA_4V096, ADS1115_PGA_2V048,
    ADS1115_PGA_1V024, ADS1115_PGA_0V512, ADS1115_PGA_0V256
} ads1115_pga;

/* Signed 16-bit conversion code -> microvolts for the given PGA. FSR_uV table
 * {6144000,4096000,2048000,1024000,512000,256000}; uV = raw * FSR_uV / 32768
 * (int64 intermediate — raw*FSR_uV exceeds int32). pga out of range -> treated as
 * ADS1115_PGA_2V048 (clamp). */
int32_t ads1115_to_microvolts(int16_t raw, ads1115_pga pga);

/* Build the 16-bit config register for a single-shot, single-ended read of
 * `channel` (0..3, masked &3) at `pga`: OS=1 (start), MUX=0b100+channel (AINx vs
 * GND), MODE=1 (single-shot), DR=0b100 (128 SPS), comparator disabled (0b00011). */
uint16_t ads1115_config(uint8_t channel, ads1115_pga pga);
```
Vectors:
- `ads1115_to_microvolts(32767, ADS1115_PGA_4V096)` → **4095875**; `(-32768, 4V096)` → **-4096000**; `(16384, 4V096)` → **2048000**; `(0, *)` → 0.
- `ads1115_config(0, ADS1115_PGA_2V048)` → **0xC583**; `ads1115_config(3, ADS1115_PGA_4V096)` → **0xF383**.

**Glue** `ads1115/ads1115_dev.{h,c}`:
```c
typedef struct { uint8_t addr; ads1115_pga pga; } ads1115;
esp_err_t ads1115_init(ads1115 *self, uint8_t addr, ads1115_pga pga);
esp_err_t ads1115_read(ads1115 *self, uint8_t channel, int32_t *microvolts);
/* write ads1115_config(channel,pga) to config reg 0x01 (2B big-endian) to start a
 * single-shot conversion, wait ~8 ms (128 SPS), read conversion reg 0x00 (2B big-
 * endian) -> int16 -> ads1115_to_microvolts. */
```
hardware_deferred: ~8 ms conversion-ready wait (first cut); real I2C bus.

## C14-3 `ina219` — current / power monitor (addr 0x40)

**Pure** `esp-libs/ina219/ina219.{h,c}`:
```c
/* Bus-voltage register (0x02): bits 15..3 in 4 mV LSB -> millivolts. */
int32_t ina219_bus_mv(uint16_t raw);                 /* (raw >> 3) * 4 */

/* Shunt-voltage register (0x01): signed, 10 uV LSB -> microvolts. */
int32_t ina219_shunt_uv(int16_t raw);                /* raw * 10 */

/* Current register (0x04): signed code * current_lsb_ua -> microamps (int64). */
int32_t ina219_current_ua(int16_t raw, uint32_t current_lsb_ua);

/* Power register (0x03): power_LSB = 20 * current_LSB -> microwatts (int64). */
int32_t ina219_power_uw(uint16_t raw, uint32_t current_lsb_ua);

/* Calibration register value = 0.04096 / (Current_LSB * R_shunt), in integer:
 * cal = 40960000 / (current_lsb_ua * shunt_milliohm) (uint64 denom). 0 if denom 0. */
uint16_t ina219_calibration(uint32_t current_lsb_ua, uint32_t shunt_milliohm);
```
Vectors:
- `ina219_bus_mv(0x1F40)` → **4000** (0x1F40=8000, >>3=1000, ×4); `ina219_shunt_uv(1000)` → 10000, `(-1000)` → -10000; `ina219_current_ua(1000,100)` → 100000; `ina219_power_uw(1000,100)` → 2_000_000; `ina219_calibration(100,100)` → **4096** (datasheet 0.1Ω/100µA example); `ina219_calibration(100,0)` → 0.

**Glue** `ina219/ina219_dev.{h,c}`:
```c
typedef struct { uint8_t addr; uint32_t current_lsb_ua; } ina219;
esp_err_t ina219_init(ina219 *self, uint8_t addr, uint32_t shunt_milliohm, uint32_t max_current_ma);
esp_err_t ina219_read_bus_mv(ina219 *self, int32_t *bus_mv);
esp_err_t ina219_read_shunt_uv(ina219 *self, int32_t *shunt_uv);
esp_err_t ina219_read_current_ua(ina219 *self, int32_t *current_ua);
esp_err_t ina219_read_power_uw(ina219 *self, int32_t *power_uw);
/* init: current_lsb_ua = max_current_ma*1000/32768; write config reg 0x00 = 0x399F
 * (32V, +/-320mV PGA, 12-bit, continuous) + calibration reg 0x05 =
 * ina219_calibration(current_lsb_ua, shunt_milliohm). reads: 2B big-endian regs
 * 0x02/0x01/0x04/0x03 -> the matching pure conversion (self->current_lsb_ua). */
```
hardware_deferred: real shunt wiring + bus; calibration accuracy on hardware.

## Testing & gates

- 3 new pure Unity suites (`test_sk6812`, `test_ads1115`, `test_ina219`) → `make -C esp-libs test` **32 → 35**.
- `make strict` (-Werror) on every pure `.c`; `make sanitize` (UBSan+ASan, `ASAN_OPTIONS=detect_leaks=0` on darwin) clean — the int64 conversions (ADS1115 µV, INA219 current/power) are the main UB-risk surface.
- All driver glue + sk6812 bit-bang: COMPILE_GATE **deferred** (xtensa absent), substituted by an adversarial reviewer pass (API consistency, register/bit-bang correctness, include resolution, NULL-safety).

## Out of scope (deferred)

- ADS1115 differential pairs, comparator, continuous mode, non-128-SPS data rates — "fuller" scope deferred per the lean-core decision.
- INA219 selectable config (16V/32V, PGA ranges, conversion times) — fixed 0x399F default.
- On-hardware run / timing validation for all three (recorded, never a DoD gate).
- Other AFE devices (ADS1015, INA226, etc.) — future cycles.

## Execution

1. `writing-plans` → implementation plan for the 3 drivers (no Task-0 prereq this cycle).
2. `embedded-sprint` → 3 parallel chains → Integration (test→35) → Sprint Review → Retro → merge (user-gated) → push → memory update.
