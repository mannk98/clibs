# esp-libs Cycle 7 — L2 bus wrappers `adc_a0` + `i2c_bus` + `spi_bus` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three compile-gated Layer-2 bus/peripheral wrappers to the `esp_libs` component — `adc_a0` (ESP8266 A0 ADC, reusing the L1 `ema`), `i2c_bus` (I2C master + register helpers), `spi_bus` (HSPI master transfer).

**Architecture:** Each wrapper is one directory of thin ESP8266-RTOS-SDK glue (`#include "driver/<bus>.h"`), compiled only by the component and exercised by symbol-touches in `esp-gate/main/main.c`. No host tests this cycle (the only pure logic — `ema`/`map_range` — is already L1-tested). Per-task GREEN = the `esp-gate` cross-compile links; runtime correctness (I2C/SPI protocol, ADC scaling) is hardware-deferred. Tasks are serial (shared `component.mk` + `esp-gate` build dir; the gate is the per-task check).

**Tech Stack:** ESP8266_RTOS_SDK v3.4 (xtensa-lx106-elf gcc via Rosetta), legacy `make`; SDK `driver/adc.h`, `driver/i2c.h`, `driver/spi.h`, `driver/gpio.h`, `freertos/FreeRTOS.h` (`pdMS_TO_TICKS`); L1 `filter.h` (`ema`).

## Global Constraints

- **Build env (run once per shell before any ESP gate build, as ONE command — `export`s do NOT persist across separate Bash calls):**
  ```sh
  export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4
  ```
  First ESP build takes a few minutes. Run `make defconfig` once if `sdkconfig` is missing.
- **Compile-gate only:** these are SDK glue → NOT host-testable. Do NOT add host tests; the verification per task is that `esp-gate` links `build/esp_libs_gate.elf`. Runtime correctness (bus protocol, ADC scaling) is hardware-deferred.
- **Naming:** `adc_a0` / `i2c_bus` / `spi_bus` (suffix avoids shadowing the SDK headers `driver/adc.h` / `driver/i2c.h` / `driver/spi.h`).
- **clibs conventions:** header guards `CLIBS_<NAME>_H`; `esp_err_t` returns; NULL/len guards; no malloc; `esp_log` diagnostics where useful; `adc_a0` reuses the L1 `ema` (no duplicated logic).
- **`component.mk`:** add each dir to all 3 lists + its `.o` to `COMPONENT_OBJS`; keep `COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip` unchanged (adc/i2c/spi are in the core `esp8266` component; the gate is the check).
- **Host gate stays 14 suites** (no new host tests): `cd esp-libs && make test` → 14 `0 Failures`, `make strict` → `strict: OK`. Do not touch existing libs, `common/`, `avr-libs/`, `esp-idf-template/`.
- **Commits:** end each message with the trailer `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `adc_a0` — A0 ADC read (+ EMA-smoothed read)

**Files:** Create `esp-libs/adc_a0/adc_a0.h`, `esp-libs/adc_a0/adc_a0.c`. Modify `esp-libs/component.mk`, `esp-gate/main/main.c`. Test: none (compile-gate; GREEN = esp-gate links).

**Interfaces:**
- Consumes: `driver/adc.h` (`adc_config_t`, `ADC_READ_TOUT_MODE`, `adc_init`, `adc_read`), `filter.h` (`ema`, `ema_update`), `esp_err.h`.
- Produces:
  ```c
  esp_err_t adc_a0_init(void);
  esp_err_t adc_a0_read(uint16_t *raw);
  esp_err_t adc_a0_read_ema(ema *filter, int32_t *out);
  ```

- [ ] **Step 1: Write `esp-libs/adc_a0/adc_a0.h`**

```c
#ifndef CLIBS_ADC_A0_H
#define CLIBS_ADC_A0_H

#include <stdint.h>
#include "esp_err.h"
#include "filter.h"   /* L1 ema */

/* Initialise the A0 (TOUT) ADC. */
esp_err_t adc_a0_init(void);
/* Read a raw sample (0..1023, unit 1/1023 V). NULL -> ESP_ERR_INVALID_ARG. */
esp_err_t adc_a0_read(uint16_t *raw);
/* Read a sample and feed it through the caller's EMA; *out gets the smoothed value. */
esp_err_t adc_a0_read_ema(ema *filter, int32_t *out);

#endif /* CLIBS_ADC_A0_H */
```

- [ ] **Step 2: Write `esp-libs/adc_a0/adc_a0.c`**

```c
#include "adc_a0.h"

#include "driver/adc.h"

esp_err_t adc_a0_init(void)
{
    adc_config_t cfg = { .mode = ADC_READ_TOUT_MODE, .clk_div = 8 };
    return adc_init(&cfg);
}

esp_err_t adc_a0_read(uint16_t *raw)
{
    if (raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_read(raw);
}

esp_err_t adc_a0_read_ema(ema *filter, int32_t *out)
{
    if (filter == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw = 0;
    esp_err_t err = adc_read(&raw);
    if (err != ESP_OK) {
        return err;
    }
    *out = ema_update(filter, (int32_t) raw);
    return ESP_OK;
}
```

- [ ] **Step 3: Wire `adc_a0` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o \
                  hysteresis/hysteresis.o crc/crc.o map_range/map_range.o \
                  median/median.o throttle/throttle.o \
                  adc_a0/adc_a0.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 4: Touch `adc_a0` in `esp-gate/main/main.c`** — add after `#include "throttle.h"`:

```c
#include "adc_a0.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* adc_a0 compile-gate. */
    (void) adc_a0_init();
    uint16_t araw = 0;
    (void) adc_a0_read(&araw);
    ema af;
    ema_init(&af, 3, 0);
    int32_t aval = 0;
    (void) adc_a0_read_ema(&af, &aval);
    ESP_LOGI(TAG, "adc raw=%u smoothed=%d", (unsigned) araw, (int) aval);
```

- [ ] **Step 5: Run both gates**

Host (still 14 suites):
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `14` and `strict: OK`.

ESP gate (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make defconfig >/dev/null && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 6: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/adc_a0/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/adc_a0): A0 ADC read + EMA-smoothed read (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: `i2c_bus` — I2C master + register helpers

**Files:** Create `esp-libs/i2c_bus/i2c_bus.h`, `esp-libs/i2c_bus/i2c_bus.c`. Modify `esp-libs/component.mk`, `esp-gate/main/main.c`. Test: none (compile-gate).

**Interfaces:**
- Consumes: `driver/i2c.h` (`i2c_config_t`, `I2C_MODE_MASTER`, `I2C_NUM_0`, `I2C_MASTER_WRITE`/`READ`, `I2C_MASTER_LAST_NACK`, `i2c_param_config`, `i2c_driver_install`, `i2c_cmd_link_create/delete`, `i2c_master_start/stop/write_byte/write/read`, `i2c_master_cmd_begin`), `driver/gpio.h` (`gpio_num_t`, `GPIO_PULLUP_ENABLE`), `freertos/FreeRTOS.h` (`pdMS_TO_TICKS`), `esp_err.h`.
- Produces:
  ```c
  esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl);
  esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
  esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
  ```

- [ ] **Step 1: Write `esp-libs/i2c_bus/i2c_bus.h`**

```c
#ifndef CLIBS_I2C_BUS_H
#define CLIBS_I2C_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"   /* gpio_num_t */

/* Configure I2C port 0 as master on the given pins (internal pull-ups on). */
esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl);
/* Write `len` bytes to register `reg` of the 7-bit `addr` device. data may be NULL iff len==0. */
esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
/* Read `len` bytes from register `reg` of the 7-bit `addr` device (repeated-start). */
esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

#endif /* CLIBS_I2C_BUS_H */
```

- [ ] **Step 2: Write `esp-libs/i2c_bus/i2c_bus.c`**

```c
#include "i2c_bus.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"   /* pdMS_TO_TICKS */

#define I2C_BUS_PORT    I2C_NUM_0
#define I2C_BUS_TIMEOUT pdMS_TO_TICKS(1000)

esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_stretch_tick = 300,
    };
    esp_err_t err = i2c_param_config(I2C_BUS_PORT, &cfg);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_BUS_PORT, I2C_MODE_MASTER);
}

esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (data == NULL && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0) {
        i2c_master_write(cmd, (uint8_t *) data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);   /* repeated start */
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_READ), true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}
```

- [ ] **Step 3: Wire `i2c_bus` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o \
                  hysteresis/hysteresis.o crc/crc.o map_range/map_range.o \
                  median/median.o throttle/throttle.o \
                  adc_a0/adc_a0.o i2c_bus/i2c_bus.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 4: Touch `i2c_bus` in `esp-gate/main/main.c`** — add after `#include "adc_a0.h"`:

```c
#include "i2c_bus.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* i2c_bus compile-gate. */
    (void) i2c_bus_init(GPIO_NUM_4, GPIO_NUM_5);
    uint8_t i2cbuf[2] = {0};
    (void) i2c_bus_write_reg(0x76, 0xF4, i2cbuf, 1);
    (void) i2c_bus_read_reg(0x76, 0xFA, i2cbuf, 2);
    ESP_LOGI(TAG, "i2c reg0=%u", (unsigned) i2cbuf[0]);
```

- [ ] **Step 5: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `14` + `strict: OK`.

ESP gate:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`. If unresolved `i2c_*`/`pdMS_TO_TICKS`, the FreeRTOS/i2c headers are in the core SDK — re-check the includes; no `COMPONENT_REQUIRES` change is expected.

- [ ] **Step 6: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/i2c_bus/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/i2c_bus): I2C master + register read/write helpers (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: `spi_bus` — HSPI master transfer

**Files:** Create `esp-libs/spi_bus/spi_bus.h`, `esp-libs/spi_bus/spi_bus.c`. Modify `esp-libs/component.mk`, `esp-gate/main/main.c`. Test: none (compile-gate).

**Interfaces:**
- Consumes: `driver/spi.h` (`spi_config_t`, `spi_trans_t`, `HSPI_HOST`, `SPI_MASTER_MODE`, `SPI_8MHz_DIV`, `spi_init`, `spi_trans`), `esp_err.h`, `<string.h>`.
- Produces:
  ```c
  esp_err_t spi_bus_init(void);
  esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len);  /* len 1..64 */
  ```

- [ ] **Step 1: Write `esp-libs/spi_bus/spi_bus.h`**

```c
#ifndef CLIBS_SPI_BUS_H
#define CLIBS_SPI_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* Initialise HSPI as master (mode 0, 8 MHz). */
esp_err_t spi_bus_init(void);
/* Full-duplex transfer of `len` bytes (1..64). rx may be NULL for write-only.
 * len 0 or > 64 -> ESP_ERR_INVALID_ARG (64 B is the SDK per-transaction limit). */
esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

#endif /* CLIBS_SPI_BUS_H */
```

- [ ] **Step 2: Write `esp-libs/spi_bus/spi_bus.c`**

```c
#include "spi_bus.h"

#include <string.h>

#include "driver/spi.h"

#define SPI_BUS_HOST   HSPI_HOST
#define SPI_BUS_MAXLEN 64

esp_err_t spi_bus_init(void)
{
    spi_config_t cfg = {0};
    cfg.interface.val = 0;
    cfg.interface.mosi_en = 1;
    cfg.interface.miso_en = 1;
    cfg.interface.cs_en = 1;
    cfg.intr_enable.val = 0;
    cfg.event_cb = NULL;
    cfg.mode = SPI_MASTER_MODE;
    cfg.clk_div = SPI_8MHz_DIV;
    return spi_init(SPI_BUS_HOST, &cfg);
}

esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (tx == NULL || len == 0 || len > SPI_BUS_MAXLEN) {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t mosi[SPI_BUS_MAXLEN / 4] = {0};
    uint32_t miso[SPI_BUS_MAXLEN / 4] = {0};
    memcpy(mosi, tx, len);

    spi_trans_t trans = {0};
    trans.mosi = mosi;
    trans.bits.mosi = (uint32_t) (len * 8);
    if (rx != NULL) {
        trans.miso = miso;
        trans.bits.miso = (uint32_t) (len * 8);
    }
    esp_err_t err = spi_trans(SPI_BUS_HOST, &trans);
    if (err == ESP_OK && rx != NULL) {
        memcpy(rx, miso, len);
    }
    return err;
}
```

- [ ] **Step 3: Wire `spi_bus` into `esp-libs/component.mk`** — replace the whole file with the FINAL version:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o \
                  hysteresis/hysteresis.o crc/crc.o map_range/map_range.o \
                  median/median.o throttle/throttle.o \
                  adc_a0/adc_a0.o i2c_bus/i2c_bus.o spi_bus/spi_bus.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
```

- [ ] **Step 4: Touch `spi_bus` in `esp-gate/main/main.c`** — add after `#include "i2c_bus.h"`:

```c
#include "spi_bus.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* spi_bus compile-gate. */
    (void) spi_bus_init();
    uint8_t spitx[4] = {0x9F, 0, 0, 0};
    uint8_t spirx[4] = {0};
    (void) spi_bus_transfer(spitx, spirx, 4);
    ESP_LOGI(TAG, "spi id0=%u", (unsigned) spirx[1]);
```

- [ ] **Step 5: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `14` + `strict: OK`.

ESP gate:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -6 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 6: Confirm the new symbols are in the ELF**

Run (PATH in the same command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; cd /Users/man.nk/git/clibs/esp-gate && xtensa-lx106-elf-nm build/esp_libs_gate.elf | grep -E " T (adc_a0_read|i2c_bus_read_reg|spi_bus_transfer)"
```
Expected: all three appear as `T` symbols.

- [ ] **Step 7: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/spi_bus/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/spi_bus): HSPI master byte transfer (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: Final gates + README update

**Files:** Modify `esp-libs/README.md`. Test: re-run both gates.

- [ ] **Step 1: Re-run the host gate**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `14` + `strict: OK`.

- [ ] **Step 2: Re-run the ESP compile-gate**

Run:
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -5 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`.

- [ ] **Step 3: Update `esp-libs/README.md`** — two edits (read the current README first to anchor exact strings):

(a) In the Layer-2 intro bullet (the `**Layer 2 — SDK wrappers …**` line that lists `wifi_sta`, `mqtt_node`, `nvs_kv`, `device_id`, `json`, `periodic`, `mdns_node`), append `, adc_a0, i2c_bus, spi_bus` to the lib list.

(b) Insert these three subsections immediately before the `### Compile-gate` subsection:
```markdown
### `adc_a0` — A0 ADC

- `adc_a0_init()` then `adc_a0_read(&raw)` (0..1023) or `adc_a0_read_ema(&filter, &out)`
  to read through an L1 `ema`. Compose with `map_range` for real units.

### `i2c_bus` — I2C master

- `i2c_bus_init(sda, scl)` then `i2c_bus_write_reg(addr, reg, data, len)` /
  `i2c_bus_read_reg(addr, reg, data, len)` (7-bit addr, repeated-start read). For
  BME280/OLED/RTC-class devices. Runtime verified on hardware later.

### `spi_bus` — HSPI master

- `spi_bus_init()` then `spi_bus_transfer(tx, rx, len)` (full-duplex, len 1..64;
  rx NULL for write-only). Runtime verified on hardware later.
```

- [ ] **Step 4: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document L2 adc_a0 + i2c_bus + spi_bus

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:** adc_a0 (Task 1), i2c_bus (Task 2), spi_bus (Task 3); component.mk + esp-gate wiring per task; README (Task 4). Host gate stays 14 (no host tests). All spec modules + infra covered. ✓

**Placeholder scan:** No TBD/TODO; every wrapper has complete header+impl; commands have expected output. ✓

**Type consistency:** Each wrapper's signatures match across its header, impl, and the esp-gate touch (`adc_a0_init/read/read_ema`, `i2c_bus_init/write_reg/read_reg`, `spi_bus_init/transfer`). `component.mk` is an incremental superset per task (cycle-6 list +adc_a0 → +i2c_bus → +spi_bus = final). `adc_a0.h` includes `filter.h` (ema) which is on the component include path. The esp-gate touch avoids the reserved identifier `asm` (uses `aval`). ✓

**SDK-API check:** `adc_config_t {mode, clk_div}` + `ADC_READ_TOUT_MODE` + `adc_init`/`adc_read` (verified). `i2c_config_t {mode, sda_io_num, sda_pullup_en, scl_io_num, scl_pullup_en, clk_stretch_tick}` + `I2C_MASTER_WRITE=0`/`READ=1` + cmd-link API + `i2c_master_cmd_begin(port, cmd, TickType_t)` (verified). `spi_config_t {interface, intr_enable, event_cb, mode, clk_div}` + `spi_trans_t {cmd,addr,mosi,miso,bits}` + `spi_init(HSPI_HOST,&cfg)`/`spi_trans` + `SPI_8MHz_DIV`/`SPI_MASTER_MODE` (verified). The `spi_trans_t.bits.mosi` 10-bit field holds up to 512 (len≤64 → ≤512). ✓
