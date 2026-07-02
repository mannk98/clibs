# esp-libs — Hardware & SDK re-verification TODO

Everything in esp-libs is **host-Unity-tested for its pure logic only**. The SDK
glue (L2 wrappers + the `*_dev` / bit-bang parts of L3 drivers + OTA) has never
run on real hardware and its **xtensa `COMPILE_GATE` is DEFERRED** because the
cross-toolchain is absent on the dev host. This file is the single checklist to
work through **when an ESP8266 + the RTOS SDK toolchain are available**.

Ground truth today: `command -v xtensa-lx106-elf-gcc` → **absent**. Any claim of
"COMPILE_GATE: pass" in commit history before that probe passes is wrong (see the
cycle-16 gate-integrity note below).

## 0. Toolchain / COMPILE_GATE (do this first — unblocks everything else)

- [ ] Install the ESP8266 RTOS SDK + xtensa toolchain + python prereqs
      (`~/esp/ESP8266_RTOS_SDK`, `~/esp/xtensa-lx106-elf`, `~/esp/idf-venv` —
      click/pyserial/cryptography<35/pyparsing<2.4/pyelftools; kconfiglib vendored).
- [ ] Run the compile-gate: `cd esp-gate && make defconfig && make` → links
      `build/esp_libs_gate.elf`. This resolves **every** L2/L3/OTA glue symbol
      against the SDK in one shot (esp-gate/main/main.c touches one symbol per lib).
- [ ] Only after a real successful link may any doc/commit record
      `COMPILE_GATE: pass`. Until then it is **deferred**.

## 1. Bit-bang / timing-critical glue (needs a scope / logic analyzer)

`ets_delay_us` is too coarse for sub-µs highs — these shipped as FIRST-CUT timing:

- [ ] `dht` (c5) — DHT22/11 1-wire-ish bit-bang read timing.
- [ ] `ds18b20` (c8) — 1-Wire reset/slot timing + CRC on a real probe.
- [ ] `servo` (c8) / `pwm_dimmer` (c5) — SW-PWM 50 Hz / duty on channel 0.
- [ ] `hcsr04` (c8) — trigger pulse + echo timeout.
- [ ] `ws2812` (c10) / `sk6812` (c14) — 800 kHz 24/32-bit stream; replace
      `ets_delay_us` with a cycle-counted (`xthal_get_ccount`) routine, ints off.
- [ ] `ir_nec` (c10) / `rc5_rx` (c12) — IR edge-capture sampling period.
- [ ] `stepper_gpio` (c12) — 28BYJ-48 half-step drive rate.
- [ ] `hx711_gpio` (c12) — SCK/DOUT bit-bang + gain pulses + DOUT-ready wait.
- [ ] `tm1637` (c15) — 2-wire CLK/DIO start/stop/ACK clocking.

## 2. I2C devices (needs a real bus + parts)

- [ ] `i2c_bus` (c7) register API + raw read/write (c13) — bus init, clock-stretch.
- [ ] `bme280` (c9) — chip-id, forced-mode read, compensation vs a reference.
- [ ] `bh1750` (c13) — ~120 ms conversion, lux accuracy.
- [ ] `sht3x` (c13) — single-shot ~15 ms, CRC, T/H accuracy.
- [ ] `ds3231` (c13) — set/get time, on-chip temp, VBAT/oscillator.
- [ ] `ads1115` (c14) — single-shot conversion-ready wait, per-channel µV accuracy.
- [ ] `ina219` (c14) — calibration vs a known shunt + load; current/power accuracy.
- [ ] `lcd1602` (c15) — HD44780 4-bit init sequence + EN-pulse settle delays (PCF8574).

## 3. SPI (needs the multi-CS wiring exercised)

- [ ] `spi_bus` multi-CS (c11 refactor) — put **2+ SPI devices** on the bus, each
      with its own software CS GPIO, and confirm no cross-talk.
- [ ] `max7219` (c11) — 7-seg/matrix frames over the shared bus (first L3 on spi_bus).

## 4. OTA end-to-end (c16) — the whole round trip

- [ ] Gateway publishes an MQTT manifest → node `ota_handle_manifest`.
- [ ] `esp_http_client` GET of the `.bin` from the LAN gateway (streaming).
- [ ] `esp_ota_ops` begin/write/end to the inactive partition (ota_0/ota_1);
      confirm the two-app partition table + bootloader partition select.
- [ ] **sha256 verify**: add the mbedtls sha256-over-image compare vs the manifest
      (currently a first-cut TODO in `ota_dev.c` — relies on esp_ota's own image
      check). This is the integrity guarantee; implement + test on hardware.
- [ ] `esp_ota_set_boot_partition` + `esp_restart` → boots the new image.
- [ ] **Rollback**: verify ESP8266 RTOS SDK v3.4 rollback semantics —
      `ota_mark_valid` on a healthy boot vs bootloader roll-back on a crash-loop
      (the SDK/bootloader valid-flag; `CONFIG_APP_ROLLBACK_ENABLE`).

## 5. Analog

- [ ] `adc_a0` (c7) — A0 0–1 V / 10-bit scaling; compose with `map_range`/`ema`.

---

## Notes / lessons carried here

- **COMPILE_GATE gate-integrity (c16):** the Integrator once wrote
  `COMPILE_GATE: pass` in a commit message while the toolchain was absent. Never
  trust a gate verdict from a message — re-probe `xtensa-lx106-elf-gcc` and record
  `deferred` unless a real cross-link succeeded. (Fixed forward in the
  embedded-workflow engine lessons; re-probe here before ticking §0.)
- Pure logic (L1 + every `*_parse`/`*_encode`/`*_render`/`*_decode`/FSM/`ota_*`
  control-plane) is already `make test` + `strict` + `sanitize` green and does
  **not** need re-testing — only the glue/timing/bus/flash items above do.
