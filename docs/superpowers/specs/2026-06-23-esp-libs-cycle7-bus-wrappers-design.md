# esp-libs cycle 7 — Layer-2 bus/peripheral wrappers: `adc_a0` + `i2c_bus` + `spi_bus` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project).

## Context

`clibs` is **libraries only**. Per the pre-hardware roadmap (A L1-libs → D CI → **C bus wrappers** → B more L3),
this cycle adds the bus/peripheral plumbing that unlocks sensors:

- **`adc_a0`** — read the ESP8266 A0 ADC; reuse the L1 `ema` filter for a smoothed read.
- **`i2c_bus`** — I2C master + register read/write helpers (BME280, OLED, RTC…).
- **`spi_bus`** — SPI (HSPI) master byte transfer.

All three are thin ESP8266-RTOS-SDK glue → **compile-gate only**. Like the L3 drivers, their **runtime
correctness (I2C/SPI protocol + ADC scaling) is hardware-deferred** — the esp-gate proves they cross-compile
and link; measuring real values waits for hardware. Names carry a suffix (`adc_a0`/`i2c_bus`/`spi_bus`) so
they never shadow the SDK headers `driver/adc.h` / `driver/i2c.h` / `driver/spi.h`.

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
| Scope | `adc_a0` + `i2c_bus` + `spi_bus`. `ota` is its own later cycle. |
| Test | compile-gate only (SDK glue). No new host suites (the pure logic — `ema`/`map_range` — is already L1-tested). |
| adc_a0 | include `adc_a0_read_ema(ema*, ...)` reusing the L1 filter (demonstrates L1 reuse, the node's real usage). |
| Naming | `adc_a0` / `i2c_bus` / `spi_bus` (avoid the SDK `driver/*.h` generic names). |
| Out of scope | `ota`, real-hardware runtime verification (i2c/spi protocol, adc scaling). |

## Architecture

```
clibs/esp-libs/
├── component.mk    # CHANGED: +3 dirs in all 3 lists + 3 .o in OBJS
├── <existing dirs> # unchanged
├── adc_a0/    adc_a0.{h,c}
├── i2c_bus/   i2c_bus.{h,c}
└── spi_bus/   spi_bus.{h,c}
clibs/esp-gate/main/main.c   # CHANGED: touch the 3 wrappers
```

Each `.c` includes its SDK driver header (`driver/adc.h` / `driver/i2c.h` / `driver/spi.h`). `adc_a0.h`
additionally includes the L1 `filter.h` (for `ema`). No malloc. `esp_err_t` returns. NULL guards.
`esp_log` diagnostics. The make build exposes the core `esp8266` component includes globally → no
`COMPONENT_REQUIRES` change expected (the gate is the check).

## `adc_a0` (`esp-libs/adc_a0/`, guard `CLIBS_ADC_A0_H`)
```c
#include "esp_err.h"
#include <stdint.h>
#include "filter.h"   /* L1 ema */

esp_err_t adc_a0_init(void);              /* adc_init({ADC_READ_TOUT_MODE, clk_div=8}) */
esp_err_t adc_a0_read(uint16_t *raw);     /* adc_read -> 0..1023 (unit 1/1023 V); NULL -> ESP_ERR_INVALID_ARG */
esp_err_t adc_a0_read_ema(ema *filter, int32_t *out);  /* adc_read -> ema_update(filter) -> *out */
```
- `adc_a0_init`: `adc_config_t c = { .mode = ADC_READ_TOUT_MODE, .clk_div = 8 }; return adc_init(&c);`.
- `adc_a0_read`: NULL guard; `return adc_read(raw);`.
- `adc_a0_read_ema`: NULL guard (filter, out); read into a local `uint16_t`, on `ESP_OK` set
  `*out = ema_update(filter, (int32_t)raw)` and return `ESP_OK`; propagate a read error otherwise.

## `i2c_bus` (`esp-libs/i2c_bus/`, guard `CLIBS_I2C_BUS_H`)
```c
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"   /* gpio_num_t */

esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl);
esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
esp_err_t i2c_bus_read_reg (uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
```
- `i2c_bus_init`: build `i2c_config_t { .mode = I2C_MODE_MASTER, .sda_io_num = sda, .sda_pullup_en =
  GPIO_PULLUP_ENABLE, .scl_io_num = scl, .scl_pullup_en = GPIO_PULLUP_ENABLE, .clk_stretch_tick = 300 }`;
  `i2c_param_config(I2C_NUM_0, &c)` then `i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER)`.
- `i2c_bus_write_reg`: NULL/`len` guards; `cmd = i2c_cmd_link_create()`; `start`; `write_byte((addr<<1)|0,
  ack_en=true)` (write bit = 0); `write_byte(reg, true)`; `write(data, len, true)`; `stop`;
  `i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000))`; `i2c_cmd_link_delete(cmd)`; return the err.
- `i2c_bus_read_reg`: `start`; `write_byte((addr<<1)|0, true)`; `write_byte(reg, true)`; `start` (repeated);
  `write_byte((addr<<1)|1, true)` (read bit = 1); `read(data, len, I2C_MASTER_LAST_NACK)`; `stop`;
  `cmd_begin` + `cmd_link_delete`; return the err.
- Uses `i2c_cmd_link_create/delete`, `i2c_master_start/stop/write_byte/write/read`, `i2c_master_cmd_begin`
  (takes a `TickType_t` timeout — `pdMS_TO_TICKS` from FreeRTOS). `addr` is the 7-bit address.

## `spi_bus` (`esp-libs/spi_bus/`, guard `CLIBS_SPI_BUS_H`)
```c
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

esp_err_t spi_bus_init(void);   /* spi_init(HSPI_HOST, master, SPI_8MHz_DIV) */
/* Full-duplex transfer of up to 64 bytes; tx/rx may be the same buffer; rx may be NULL (write-only). */
esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len);
```
- `spi_bus_init`: `spi_config_t c = {0}; c.interface.val = 0; c.interface.mosi_en = 1; c.interface.miso_en
  = 1; c.interface.cs_en = 1; c.mode = SPI_MASTER_MODE; c.clk_div = SPI_8MHz_DIV; c.intr_enable.val = 0;
  c.event_cb = NULL; return spi_init(HSPI_HOST, &c);`.
- `spi_bus_transfer`: NULL `tx` / `len==0` / `len > 64` → `ESP_ERR_INVALID_ARG`. Pack `tx` into a local
  `uint32_t mosi[16]` (64 bytes), build `spi_trans_t t = {0}; t.mosi = mosi; t.bits.mosi = len*8; if (rx)
  { t.miso = miso; t.bits.miso = len*8; }`; `esp_err_t e = spi_trans(HSPI_HOST, &t);` on `ESP_OK` and `rx`
  copy `miso` bytes back into `rx`. (The 64-byte cap is the SDK per-transaction limit via this API; documented.)

## Build infrastructure changes
- **`esp-libs/component.mk`:** add `adc_a0 i2c_bus spi_bus` to `COMPONENT_ADD_INCLUDEDIRS` +
  `COMPONENT_SRCDIRS`; add `adc_a0/adc_a0.o i2c_bus/i2c_bus.o spi_bus/spi_bus.o` to `COMPONENT_OBJS`.
  `test_*.c` n/a (none); `COMPONENT_REQUIRES` unchanged.
- **`esp-gate/main/main.c`:** include the 3 headers + a touch block calling one+ symbol of each.
- **`esp-libs/Makefile`:** unchanged (no host tests this cycle — still 14 suites).

## Testing
- **Host:** `make test` unchanged → 14 suites all `0 Failures`; `make strict` → `strict: OK`.
- **ESP compile-gate:** `cd esp-gate && make` → links `build/esp_libs_gate.elf` with the new symbols
  (`adc_a0_read`, `i2c_bus_read_reg`, `spi_bus_transfer`) present as `T`.
- **Hardware: out of scope.** I2C/SPI protocol + ADC scaling are verified on real hardware later.

## Conventions & scope
- SDK glue: `.c` includes the SDK driver header; `esp_err_t` returns; NULL/len guards; no malloc;
  `CLIBS_*_H` guards; `esp_log` diagnostics; `adc_a0` reuses L1 `ema` (no duplicated logic).
- Naming suffix avoids the SDK `driver/*.h` generic names.
- Out of scope: `ota`, real-hardware runtime verification.

## Build sequence (subagent-driven, serial — shared component.mk/esp-gate + the gate is the per-task check)
1. `adc_a0` — wrapper + wire `component.mk` + `esp-gate/main.c`; esp-gate links; host still 14 suites.
2. `i2c_bus` — wrapper + wire; esp-gate links.
3. `spi_bus` — wrapper + wire; esp-gate links + symbol check.
4. Final: host gate (14) + esp-gate link + symbol check; README L2 section update.
5. (clibs feature branch → merge → push.)
