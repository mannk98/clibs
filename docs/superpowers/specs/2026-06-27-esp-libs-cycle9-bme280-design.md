# esp-libs cycle 9 — `bme280` L3 driver (T/H/P over I2C) (design)

Date: 2026-06-27
Location: `clibs/esp-libs/` (the ESP8266 component) + `clibs/esp-gate/` (compile-gate).

## Context

First I²C **sensor** driver in esp-libs, and the first L3 driver built on an L2 **bus** wrapper
(`i2c_bus`, cycle 7). The Bosch BME280 reports temperature, humidity, and pressure; its value over a
DHT22 is the compensated, calibrated readings via Bosch's fixed-point compensation formulas. Following
the established L3 header-split (`<lib>_parse.*` pure + `<lib>.*` SDK glue, like `ds18b20`), the
**compensation + calibration parsing is pure and host-Unity-tested**; the I²C glue is compile-gate-only.

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Measurands | all three — T + H + P (full BME280). |
| Output units | **SI-friendly**: `temp_mc` milli-°C (int32), `humi_mrh` milli-%RH (uint32), `press_pa` Pa (uint32). Compensation computes Bosch-native then converts. |
| Compensation variant | Bosch **integer** formulas: T int32, **P int64** (the accurate variant), H int32. |
| Split | two pure functions — `bme280_parse_calib` (register bytes → calib struct) + `bme280_compensate` (calib + adc → SI). Glue extracts adc from the 8 raw bytes. |
| Bus | depends on `i2c_bus` (cycle 7); caller runs `i2c_bus_init` first. |
| Mode | **forced** (one-shot) — ideal for a polled node; oversampling ×1 each. |
| Address | `0x76` primary / `0x77` secondary (caller passes). |
| Test oracle | vectors cross-checked: Bosch **integer** vs **double** formulas must agree (they do — see below). |
| Out of scope | `normal` (continuous) mode, IIR filter config, SPI, the on-target hardware run. |

## Architecture

```
clibs/esp-libs/
├── Makefile        # CHANGED: + bme280_parse host-test target (test -> 19 suites) + strict line
├── component.mk    # CHANGED: + bme280 dir (3 lists) + bme280/bme280.o + bme280/bme280_parse.o
├── bme280/
│   ├── bme280_parse.h   bme280_parse.c   # pure: parse_calib + compensate (host-tested)
│   ├── bme280.h         bme280.c         # SDK glue over i2c_bus (compile-gated, deferred)
│   └── test_bme280_parse.c               # Unity host test (the pure part)
clibs/esp-gate/main/main.c   # CHANGED: include bme280.h + touch one symbol
```

Pure part: `<stdint.h>`/`<stdbool.h>` only (no SDK), host-testable. Glue: `esp_err.h` + `i2c_bus.h`.
Conventions = clibs standard (transparent struct + `self`-methods, `CLIBS_*_H` guards, NULL guards,
doc comments, "Not reentrant", no-malloc, no UB — the P int64 / H int32 intermediates stay in range).

## Module specs

### `bme280_parse` (`CLIBS_BME280_PARSE_H`, `<stdint.h>`,`<stdbool.h>`)
```c
typedef struct {                 /* Bosch trimming coefficients (fields private) */
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} bme280_calib;

typedef struct { int32_t temp_mc; uint32_t humi_mrh; uint32_t press_pa; } bme280_reading;

/* Parse calib register blocks into `out`: calib26 = 0x88..0xA1 (26 bytes), calibH = 0xE1..0xE7 (7 bytes).
 * Handles the packed 12-bit signed H4/H5:
 *   H4 = (calibH[3] << 4) | (calibH[4] & 0x0F);  H5 = (calibH[5] << 4) | (calibH[4] >> 4);  (sign-extend 12->16)
 * NULL any arg -> false. Pure / host-testable. */
bool bme280_parse_calib(const uint8_t calib26[26], const uint8_t calibH[7], bme280_calib *out);

/* Bosch integer compensation (datasheet 4.2.3): T int32 + P int64 + H int32, converted to SI:
 *   temp_mc  = T_0.01C * 10;  press_pa = P_Q24.8 >> 8;  humi_mrh = (H_Q22.10 * 1000) >> 10.
 * adc_T/adc_P are 20-bit, adc_H 16-bit (caller/glue extracts from raw regs). NULL -> false. Pure. */
bool bme280_compensate(const bme280_calib *cal, int32_t adc_T, int32_t adc_P, int32_t adc_H, bme280_reading *out);
```

**Host-test vectors** (cross-checked: Bosch integer ≈ Bosch double — temp Δ0.0003 °C, pressure Δ0.59 Pa
from the `>>8` truncation, humidity Δ0.0057 %RH):

```
calib26[26] = {0x45,0x6F,0x6F,0x68,0x32,0x00,0xF4,0x93,0x43,0xD6,0xD0,0x0B,0xBC,0x18,
               0xEB,0xFF,0xF9,0xFF,0x8C,0x3C,0xF8,0xC6,0x70,0x17,0x00,0x4B}
calibH[7]   = {0x72,0x01,0x00,0x12,0x02,0x00,0x1E}
```
- `bme280_parse_calib` → `T1=28485, T2=26735, T3=50, P1=37876, P2=-10685, P3=3024, P4=6332, P5=-21,
  P6=-7, P7=15500, P8=-14600, P9=6000, H1=75, H2=370, H3=0, H4=290, H5=0, H6=30`.
- **H4/H5 sign-extension sub-case** (12-bit signed): for `calibH[3..5] = {0xFC, 0xCE, 0xF9}` →
  `H4 == -50`, `H5 == -100` (proves the 12→16-bit sign extension, which the primary vector's positive
  H4/H5 does not).
- `bme280_compensate(cal, adc_T=519888, adc_P=326816, adc_H=26083, &r)` →
  `r.temp_mc == 20440` (20.44 °C), `r.press_pa == 101576` (1015.76 hPa), `r.humi_mrh == 42743` (42.743 %RH).
  (Bosch-native intermediates for reference: `t_fine=104653`, `T=2044` (0.01 °C), `P=26003595` (Q24.8),
  `H=43769` (Q22.10).)
- NULL: `bme280_parse_calib(NULL,…)`/`(…,NULL,…)`/`(…,NULL)` → false; `bme280_compensate(NULL,…)` /
  `(…, NULL out)` → false.

### `bme280` — SDK glue (`CLIBS_BME280_H`, `<stdint.h>`,`"esp_err.h"`,`"bme280_parse.h"`)
```c
#define BME280_ADDR_PRIMARY   0x76
#define BME280_ADDR_SECONDARY 0x77

typedef struct { uint8_t addr; bme280_calib calib; } bme280;

/* Probe chip id (0xD0 must read 0x60), read+parse the two calib blocks, set ctrl_hum (0xF2) osrs_h x1
 * BEFORE ctrl_meas (datasheet ordering quirk). Uses i2c_bus (call i2c_bus_init first).
 * ESP_OK / ESP_ERR_NOT_FOUND (bad chip id) / propagated i2c_bus esp_err. */
esp_err_t bme280_init(bme280 *self, uint8_t addr);

/* Forced one-shot: write ctrl_meas (0xF4) = osrs_t x1 | osrs_p x1 | mode=forced(0b01); wait for the
 * measurement (poll status 0xF3 bit3, bounded) ; read 8 raw bytes (0xF7..0xFE); extract adc_P (20b),
 * adc_T (20b), adc_H (16b); call bme280_compensate -> out. ESP_OK / propagated esp_err. */
esp_err_t bme280_read(bme280 *self, bme280_reading *out);
```
- raw-byte extraction (in glue): `adc_P = (raw[0]<<12)|(raw[1]<<4)|(raw[2]>>4)`;
  `adc_T = (raw[3]<<12)|(raw[4]<<4)|(raw[5]>>4)`; `adc_H = (raw[6]<<8)|raw[7]`.
- The glue needs the real SDK (`i2c_bus` → `driver/i2c`, `esp_err.h`) → **not host-runnable**.

## Testing
- **Host (real RED→GREEN):** `cd esp-libs && make test` → **19 suites** (18 existing + `bme280_parse`),
  all `0 Failures`; `make strict` → `strict: OK` (`bme280_parse.c` -Werror clean).
- **ESP compile-gate:** `cd esp-gate && make` links `bme280.c` against the SDK — **DEFERRED**
  (xtensa toolchain absent on this host); re-run on an SDK-equipped host. Records GATE state, not pass.
- **Hardware:** deferred (real BME280 on the I²C bus) — verify forced-mode timing + actual readings.

## Build infrastructure changes
- **`esp-libs/Makefile`:** add `bme280_parse` to `.PHONY` + the `test:` aggregate (→ 19), a run target
  (`$(CC) $(CFLAGS) -I$(UNITY) -Ibme280 -o $(BUILD)/test_bme280_parse bme280/test_bme280_parse.c
  bme280/bme280_parse.c $(UNITY)/unity.c && ./…`), and a `strict:` `-Werror` compile of `bme280_parse.c`.
- **`esp-libs/component.mk`:** add `bme280` to `COMPONENT_ADD_INCLUDEDIRS` + `COMPONENT_SRCDIRS`, and
  `bme280/bme280.o bme280/bme280_parse.o` to `COMPONENT_OBJS` (test file excluded).
- **`esp-gate/main/main.c`:** `#include "bme280.h"` + a touch block calling one symbol (e.g.
  `bme280_compensate` on a zeroed calib, or reference `bme280_init`).
- **`esp-libs/README.md`:** add an L3 row for `bme280`.

## Conventions & scope
- L3 header-split (pure parse/compensate host-tested; SDK glue compile-gated). No malloc, transparent
  struct + `self`-methods, `CLIBS_*_H` guards, NULL/edge guards, doc comment per public function,
  "Not reentrant". No UB (P int64 / H int32 intermediates in range; `make sanitize` is common/-only, so
  run a manual `-fsanitize=undefined` pass on the host test in review).
- Out of scope: normal mode, IIR filter, SPI, on-target hardware run.

## Build sequence
1. `bme280_parse` — RED test (parse_calib + compensate vectors above) → impl → GREEN + strict.
2. `bme280` glue — write `bme280.{h,c}` (init/read over i2c_bus); compile-gate DEFERRED.
3. Integration: wire `Makefile` (test→19) + `component.mk` + `esp-gate/main.c` + `README`; `make test`
   + `make strict`; attempt `esp-gate` link (deferred note if toolchain absent).
4. clibs feature branch `feat/esp-libs-cycle9-bme280` → merge → push (user's call).
