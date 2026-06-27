# esp-libs cycle 13 — I2C sensors + RTC (bh1750, sht3x, ds3231) Design

**Status:** approved (brainstorm 2026-06-27)

**Goal:** Add three independent L3 I2C devices to esp-libs for ESP8266 smart-home nodes — `bh1750` (ambient light), `sht3x` (temperature/humidity), `ds3231` (real-time clock) — each header-split into a host-Unity-tested pure part + SDK glue. Plus a small `i2c_bus` extension (raw read/write) that the two command-based sensors need.

## Background

esp-libs currently has only one I2C sensor (`bme280`). `i2c_bus` (cycle 7) is **register-oriented only**: `i2c_bus_write_reg(addr, reg, data, len)` and `i2c_bus_read_reg(addr, reg, data, len)` always write a register/pointer byte before the read. That fits register-mapped devices (DS3231, BME280) but NOT command-based devices:

- **BH1750** — write a single opcode byte (mode), then read 2 data bytes with **no register pointer**.
- **SHT3x** — write a 16-bit command (2 bytes), then read 6 data bytes with **no register pointer**.

So the cycle needs a raw read primitive (and, for symmetry/clarity, a raw write) on `i2c_bus`. DS3231 is register-mapped and uses the existing API unchanged.

Units follow the bme280 convention: milli-units (`temp_mc` milli-°C, `humi_mrh` milli-%RH, `milli_lux`).

## Architecture

Two execution stages (mirrors how the spi_bus multi-CS refactor preceded would-be SPI drivers):

1. **Pre-step — `i2c_bus` raw read/write (direct, glue-only).** Add `i2c_bus_read` + `i2c_bus_write` (no register). This is L2 SDK glue with **no host-test surface**, so it is done as a direct change (edit → host-gate regression check → adversarial review substituting the deferred COMPILE_GATE → commit on the cycle branch), NOT as an embedded-sprint story (an L2-only story has no RED test). Committed first so the sprint drivers can consume it.

2. **embedded-sprint — three L3 drivers in parallel.** Each is header-split: a pure `*.{h,c}` (host-Unity-tested) + SDK glue `*.{h,c}` (xtensa COMPILE_GATE **deferred** — `xtensa-lx106-elf-gcc` absent). After the pre-step, all three stories have `deps: []` and run in parallel (`Tester RED → Dev GREEN → Reviewer`). Integration wires `esp-libs/Makefile` (test 29 → **32**), `component.mk`, `esp-gate/main/main.c`, `README`.

### Header-split & conventions (per profile-clibs)

- Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`), no SDK, no malloc, host-testable.
- Glue files include `esp_err.h` + the `i2c_bus.h` shared bus; not host-runnable.
- `CLIBS_<MODULE>_H` guards; NULL-guard every public pointer param; doc comment per public fn; "Not reentrant".
- **No UB:** shifts only on positive uint8/uint16/uint32 operands; SHT3x conversion widens to int64 before the multiply (the map_range "widen-before-multiply" rule); no left-shift of a possibly-negative value.

## Pre-step: `i2c_bus` raw read/write

Add to `esp-libs/i2c_bus/i2c_bus.{h,c}`:

```c
/* Raw write: START, addr+W, data[0..len-1], STOP. No register/pointer byte.
 * data may be NULL iff len == 0 (address-only probe). */
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);

/* Raw read: START, addr+R, read len bytes (last NACK), STOP. No register write first. */
esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len);
```

- `i2c_bus_read`: same cmd-link shape as `i2c_bus_read_reg` minus the register write + first repeated-start. `data == NULL || len == 0 → ESP_ERR_INVALID_ARG`.
- `i2c_bus_write`: same shape as `i2c_bus_write_reg` minus the register byte. `data == NULL && len > 0 → ESP_ERR_INVALID_ARG`.
- Existing `i2c_bus_write_reg` / `i2c_bus_read_reg` unchanged (DS3231 + bme280 keep using them).
- esp-gate adds a raw-read + raw-write symbol touch.

## C13-1 `bh1750` — ambient light (I2C 0x23 / 0x5C)

**Pure** `esp-libs/bh1750/bh1750.{h,c}`:
```c
/* 16-bit big-endian raw count (raw[0]=MSB) -> milli-lux. lux = raw / 1.2, so
 * milli_lux = raw * 2500 / 3 (exact in uint32: 65535*2500 < 2^32). NULL -> false. */
bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux);
```
Vectors: `{0x00,0x06}`→ val 6 → **5000** mlx (5 lux); `{0x00,0x0C}`→ 12 → **10000** mlx; `{0xFF,0xFF}`→ 65535 → **54_612_500** mlx; NULL → false.

**Glue** `bh1750/bh1750_dev.{h,c}` (or `bh1750.{h,c}` glue split — pure named `bh1750.*`, glue named `bh1750_dev.*`):
```c
typedef struct { uint8_t addr; } bh1750;
esp_err_t bh1750_init(bh1750 *self, uint8_t addr);        /* i2c_bus_write the continuous-HR-mode opcode 0x10 */
esp_err_t bh1750_read(bh1750 *self, uint32_t *milli_lux); /* i2c_bus_read 2 bytes -> bh1750_parse */
```
hardware_deferred: ~120 ms conversion time after mode set / between reads (first cut; tune on hardware); real I2C bus.

## C13-2 `sht3x` — temperature + humidity (I2C 0x44 / 0x45)

**Pure** `esp-libs/sht3x/sht3x.{h,c}`:
```c
/* 6-byte frame: [t_msb,t_lsb,t_crc, h_msb,h_lsb,h_crc]. Validate both Sensirion
 * CRC8 (poly 0x31, init 0xFF, no final xor, MSB-first) over each 2-byte group;
 * any mismatch -> false. Then (int64 intermediate, no overflow / no signed-shift):
 *   temp_mc  = -45000 + (int32_t)(175000LL * raw_t / 65535)
 *   humi_mrh =          (int32_t)(100000LL * raw_h / 65535)
 * raw_t = (t_msb<<8)|t_lsb, raw_h = (h_msb<<8)|h_lsb. NULL -> false. */
bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh);
```
Sensirion CRC8 canonical vectors (anchor the RED test): `CRC8({0x00,0x00}) = 0x81`, `CRC8({0xBE,0xEF}) = 0x92` (datasheet example).
Vectors:
- `{0x00,0x00,0x81, 0x00,0x00,0x81}` → true, temp_mc **-45000**, humi_mrh **0**.
- `{0xBE,0xEF,0x92, 0xBE,0xEF,0x92}` → true, raw 0xBEEF=48879 → temp_mc **85523**, humi_mrh **74584**.
- `{0x00,0x00,0x00, 0x00,0x00,0x81}` → false (first CRC wrong).
- NULL (any out-param NULL too) → false.

**Glue** `sht3x/sht3x_dev.{h,c}`:
```c
typedef struct { uint8_t addr; } sht3x;
esp_err_t sht3x_init(sht3x *self, uint8_t addr);
esp_err_t sht3x_read(sht3x *self, int32_t *temp_mc, int32_t *humi_mrh);
/* write cmd {0x24,0x00} (single-shot, high-repeatability, clock-stretch off) via
 * i2c_bus_write, wait ~15 ms, i2c_bus_read 6 bytes -> sht3x_parse. */
```
hardware_deferred: ~15 ms measurement delay (first cut); real I2C bus.

## C13-3 `ds3231` — real-time clock (I2C 0x68) — register-based, no pre-step dep

**Pure** `esp-libs/ds3231/ds3231.{h,c}`:
```c
typedef struct { uint8_t sec, min, hour, dow, date, month; uint16_t year; } ds3231_time;

/* Decode the 7 timekeeping registers (0x00..0x06, all BCD; 24-hour mode):
 *   sec=bcd(raw[0]&0x7F) min=bcd(raw[1]&0x7F) hour=bcd(raw[2]&0x3F)
 *   dow=raw[3]&0x07 date=bcd(raw[4]&0x3F) month=bcd(raw[5]&0x1F) year=2000+bcd(raw[6])
 * NULL -> false. */
bool ds3231_decode(const uint8_t raw[7], ds3231_time *out);

/* Inverse: encode a ds3231_time into 7 BCD registers (year %100). NULL -> false. */
bool ds3231_encode(const ds3231_time *t, uint8_t raw[7]);

/* On-chip temperature regs 0x11 (signed integer °C) + 0x12 (bits 7..6 = quarters):
 *   temp_mc = (int8_t)msb * 1000 + (lsb >> 6) * 250. */
int32_t ds3231_temp_mc(uint8_t msb, uint8_t lsb);
```
Vectors:
- decode `{0x30,0x45,0x13,0x06,0x27,0x06,0x26}` → {sec30,min45,hour13,dow6,date27,month6,year2026}; encode that struct → same 7 bytes (round-trip).
- `ds3231_temp_mc(0x19,0x40)` → **25250** (25.25 °C); `ds3231_temp_mc(0xFF,0x80)` → **-500** (-0.5 °C, signed msb); NULL → false.

**Glue** `ds3231/ds3231_dev.{h,c}` — uses the EXISTING register API:
```c
typedef struct { uint8_t addr; } ds3231;
esp_err_t ds3231_init(ds3231 *self, uint8_t addr);               /* 0x68 */
esp_err_t ds3231_get(ds3231 *self, ds3231_time *out);            /* read_reg 0x00, 7 bytes -> decode */
esp_err_t ds3231_set(ds3231 *self, const ds3231_time *t);        /* encode -> write_reg 0x00, 7 bytes */
esp_err_t ds3231_read_temp(ds3231 *self, int32_t *temp_mc);      /* read_reg 0x11, 2 bytes -> ds3231_temp_mc */
```
hardware_deferred: oscillator/battery wiring; real I2C bus.

## Naming note

To keep the pure file host-linkable under its own Makefile target and matching the cycle-12 precedent (`stepper.c` pure / `stepper_gpio.c` glue), each driver's **pure** part takes the bare lib name (`bh1750.c`, `sht3x.c`, `ds3231.c`) and the **glue** takes a `_dev` suffix (`bh1750_dev.c`, `sht3x_dev.c`, `ds3231_dev.c`). The writing-plans step finalizes exact filenames; the host suite links only the pure `*.c`.

## Testing & gates

- 3 new pure Unity suites (`test_bh1750`, `test_sht3x`, `test_ds3231`) → `make -C esp-libs test` **29 → 32**.
- `make strict` (-Werror) on every pure `.c`; `make sanitize` (UBSan+ASan, `ASAN_OPTIONS=detect_leaks=0` on darwin) clean — SHT3x's int64 conversion + CRC loop is the main UB-risk surface.
- The `i2c_bus` pre-step + all driver glue: COMPILE_GATE **deferred** (xtensa absent), substituted by an adversarial reviewer pass (API consistency, I2C cmd-link correctness, include resolution, NULL-safety).

## Out of scope (deferred)

- DS3231 alarms (2 alarms + INT/SQW), SHT3x heater control, BH1750 selectable modes (lo-res / one-shot) — "Fuller" scope, deferred per the lean-core decision.
- On-hardware run / I2C timing validation for all three (recorded, never a DoD gate).
- Other I2C devices (ADS1115, INA219, LCD1602, etc.) — future cycles.

## Execution

1. Pre-step `i2c_bus` raw read/write — direct (this session), committed on branch `feat/esp-libs-cycle13-i2c`.
2. `writing-plans` → implementation plan for the 3 drivers.
3. `embedded-sprint` → 3 parallel chains → Integration (test→32) → Sprint Review → Retro → merge (user-gated) → push → memory update.
