# esp-libs

Reusable C building blocks for **ESP8266** smart-home nodes (ESP8266 RTOS SDK,
FreeRTOS). Built bottom-up:

- **Layer 1 — pure logic (this set):** plain portable C, host-unit-tested with
  Unity, no SDK/hardware needed (compiles on ESP8266 unchanged).
- **Layer 2 — SDK wrappers (this set):** `wifi_sta`, `mqtt_node`, `nvs_kv`,
  `device_id`, `json`, `periodic`, `mdns_node`, `adc_a0`, `i2c_bus`, `spi_bus` — ESP8266-RTOS-SDK glue verified by
  the `esp-gate` link; the pure parts (`device_id_format`, the `json` builder) are
  host-Unity-tested.
- **Layer 3 — device drivers (this set):** `relay`, `button`, `dht`, `pwm_dimmer`, `ds18b20`, `servo`, `hcsr04`, `bme280`, `ws2812`, `ir_nec`, `pir`, `ssd1306`, `max7219`, `rotary` —
  GPIO/PWM/bit-bang/I2C/SPI glue verified by the `esp-gate` link; the pure parts
  (`relay_level`, `dht_parse`, `dimmer_duty`, `bme280_parse`/`bme280_compensate`, `ws2812_render`, `ir_nec_decode`, `pir` FSM, `ssd1306_fb`, `max7219_encode`, `rotary` decode) are host-Unity-tested. **Hardware run
  is deferred** — bit-bang timing (dht, ws2812, ir_nec) is a first cut to tune on real hardware.

## Layer-1 libraries

| Lib | What it does |
|-----|--------------|
| `backoff` | Exponential reconnect backoff for WiFi/MQTT (`backoff_init/next/reset`). |
| `debounce` | Input debounce with edge detection (`debounce_init/update/level`). |
| `filter` | Integer EMA smoothing for ADC (`ema_init/update`, alpha = 1/2^shift, no floats). |
| `mqtt_topic` | Topic join + MQTT `+`/`#` wildcard match (`mqtt_topic_join/match`). |
| `hysteresis` | Schmitt-trigger deadband on/off control (`hysteresis_init/update/state`). |
| `crc` | `crc8_maxim` (1-Wire) + `crc16_modbus` checksums — **re-exports `common/crc`** (single source of truth; also brings `crc16_ccitt`/`crc32`). |
| `map_range` | Integer linear map + `clamp_i32` (ADC counts → real units). |
| `median` | `median3` despike (median of three). |
| `throttle` | Minimum-interval gate (`throttle_init/allow`) for rate-limiting. |
| `saturating_counter` | Bounded counter over `[min,max]` that clamps instead of wrapping (`saturating_counter_init/inc/dec/get`). |

Each is freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`, +`<string.h>` in
`mqtt_topic`), static / no-malloc, transparent struct + `self`-methods.

## Tests

Host unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity), reused from
`../common/third_party/unity`.

```sh
make test     # build + run all host suites (25 currently)
make strict   # -Werror warning gate
make clean
```

## Layer 2 — ESP8266 component (SDK wrappers)

`esp-libs` is also an **ESP8266 RTOS SDK component** via `component.mk` at the
root of this directory. Drop the `esp-libs/` folder (or symlink it) into your
project's `components/` tree and the SDK build picks it up automatically.

### `wifi_sta` — first L2 wrapper

`wifi_sta` manages ESP8266 station mode with automatic backoff reconnect:

- `wifi_sta_start(cfg)` — initialise the netif/event-loop, register reconnect
  task, and call `esp_wifi_connect()`.
- `wifi_sta_stop()` — tear down gracefully.
- `wifi_sta_get_state()` — returns the current `wifi_sta_state_t`
  (`WIFI_STA_DISCONNECTED / CONNECTING / CONNECTED`).

The reconnect task consumes `backoff_init/next/reset` from Layer 1 — no extra
dependency.

### `nvs_kv` — NVS key/value store

Reusable key/value wrapper over one NVS namespace, hiding the SDK's
open/probe-size/commit/close churn. Caller-provided `nvs_kv` storage, no malloc.

- `nvs_kv_init()` — initialise flash once (erases + retries on a version/space error).
- `nvs_kv_open(&kv, "node", true)` — open a namespace read-write or read-only.
- `nvs_kv_get_str/_u32(..., def)` — read with a default for a missing key.
- `nvs_kv_set_str/_u32(...)` then `nvs_kv_commit(&kv)` — write and flush.
- `nvs_kv_close(&kv)`.

### `mqtt_node` — MQTT manager with presence

Singleton MQTT client for a node, built on the SDK's esp-mqtt (named `mqtt_node`
to avoid colliding with the SDK header `mqtt_client.h`). Relies on esp-mqtt's
built-in auto-reconnect.

- `mqtt_node_start(cfg)` — connect to `cfg.uri`; if `cfg.presence_topic` is set,
  publishes a retained `online_msg` on connect and registers `offline_msg` as the
  Last-Will so the broker announces the node going offline.
- `mqtt_node_publish/_subscribe(...)`, `mqtt_node_stop()`, `mqtt_node_get_state()`.
- `cfg.state_cb` reports CONNECTED/DISCONNECTED; `cfg.msg_cb` delivers inbound
  messages (topic/data with explicit lengths — not NUL-terminated).

### `device_id` — stable id from the MAC

- `device_id_format(mac, buf, n, "esp-")` — pure, host-tested: last 3 MAC bytes as
  hex after a prefix (e.g. `"esp-3c71bf"`).
- `device_id_get(buf, n, "esp-")` — reads the station MAC and formats it (use for
  the MQTT `client_id`, a default `node_id`, or a topic prefix).

### `json` — payload builder + command getters

- `json_build` (pure, host-tested, no malloc): `json_build_init/_str/_int/_raw/_end`
  builds a small JSON object like `{"state":"on","rssi":-60}` into a fixed buffer;
  `_end` returns the length or -1 on overflow.
- `json_get_str/_int(json, key, …, def)` (cJSON) — read a field from an inbound
  command payload with a default.

### `periodic` — esp_timer periodic callback

- `periodic_start(&p, period_ms, cb, ctx)` / `periodic_stop(&p)` — fire `cb(ctx)`
  every `period_ms` (telemetry cadence). The callback runs in the esp_timer task —
  keep it short.

### `mdns_node` — mDNS

- `mdns_node_init("node-1")` — start mDNS and set the hostname.
- `mdns_node_resolve("broker.local", ip, sizeof ip, 1000)` — resolve a host to a
  dotted-quad IP. (Named `mdns_node` to avoid the SDK header `mdns.h`.)

### `relay` — GPIO output actuator

- `relay_init(&r, GPIO_NUM_5, active_low)` / `relay_set(&r, on)` / `relay_toggle(&r)` /
  `relay_is_on(&r)`. `relay_level(on, active_low)` (pure, host-tested) maps the on-state
  to the GPIO level.

### `button` — debounced GPIO input

- `button_init(&b, pin, active_low, threshold)` then `button_poll(&b)` at a fixed
  interval → `BUTTON_PRESSED` / `BUTTON_RELEASED` / `BUTTON_NONE`. Reuses the L1
  `debounce` FSM. `button_is_pressed(&b)` for the current state.

### `dht` — DHT22 / DHT11 temperature + humidity

- `dht_parse(frame, &temp_dc, &humi_dpct)` (pure, host-tested): a 5-byte frame →
  deci-°C / deci-%RH with checksum validation.
- `dht_read(&d, &temp_dc, &humi_dpct)` bit-bangs a read (`ESP_OK` /
  `ESP_ERR_TIMEOUT` / `ESP_ERR_INVALID_CRC`). The bit-bang timing is a first cut —
  tune on hardware.

### `pwm_dimmer` — software PWM dimmer

- `pwm_dimmer_init(&d, pin, period_us)` then `pwm_dimmer_set(&d, percent)`.
  `dimmer_duty(percent, max_duty)` (pure, host-tested) maps 0–100 % to a duty.
  NOTE: the SDK PWM is a single global subsystem — one dimmer on channel 0.

### `adc_a0` — A0 ADC

- `adc_a0_init()` then `adc_a0_read(&raw)` (0..1023) or `adc_a0_read_ema(&filter, &out)`
  to read through an L1 `ema`. Compose with `map_range` for real units.

### `i2c_bus` — I2C master

- `i2c_bus_init(sda, scl)` then `i2c_bus_write_reg(addr, reg, data, len)` /
  `i2c_bus_read_reg(addr, reg, data, len)` (7-bit addr, repeated-start read). For
  BME280/OLED/RTC-class devices. Runtime verified on hardware later.
- `i2c_bus_write(addr, data, len)` / `i2c_bus_read(addr, data, len)` — raw transfers
  with no register/pointer byte, for command-based devices (BH1750, SHT3x).

### `spi_bus` — HSPI master + multi-device software CS

- `spi_bus_init()` disables the single hardware CS, then `spi_bus_transfer(tx, rx, len)`
  is a raw full-duplex transfer with no CS (len 1..64; rx NULL for write-only).
- `spi_device_init(&dev, cs_pin)` + `spi_device_transfer(&dev, tx, rx, len)` give each
  peripheral its own active-low software CS GPIO, so multiple SPI devices share the
  bus (ESP8266 HSPI has only one hardware CS). Runtime verified on hardware later.

### `ds18b20` — 1-Wire temperature

- `ds18b20_parse_temp(scratch, &temp_mc)` (pure, host-tested): validate the 9-byte
  scratchpad via `crc8_maxim` and convert to milli-°C.
- `ds18b20_init(&d, pin)` then `ds18b20_read(&d, &temp_mc)` bit-bangs a single-drop
  1-Wire read. Timing is a first cut — tune on hardware.

### `servo` — PWM servo

- `servo_angle_to_duty(angle)` (pure, host-tested): 0–180° → 1000–2000 µs pulse.
- `servo_init(&s, pin)` (50 Hz) then `servo_set(&s, angle)`. Shares the single global
  PWM with `pwm_dimmer` — only one at a time.

### `hcsr04` — ultrasonic distance

- `hcsr04_us_to_mm(echo_us)` (pure, host-tested): echo time → mm.
- `hcsr04_init(&h, trig, echo)` then `hcsr04_read(&h, &mm)` (trigger + bounded echo
  measure → `ESP_ERR_TIMEOUT` on no echo). Timing tuned on hardware.

### `bme280` — T/H/P over I2C

- `bme280_parse_calib` + `bme280_compensate` (pure, host-tested): Bosch fixed-point
  compensation → SI units (milli-°C / milli-%RH / Pa).
- `bme280_init(&d, BME280_ADDR_PRIMARY)` then `bme280_read(&d, &r)` — forced-mode
  one-shot over `i2c_bus`. Runtime verified on hardware later.

### `ws2812` — WS2812/NeoPixel RGB

- `ws2812_render(px, n, brightness, out, out_size)` (pure, host-tested): RGB → GRB byte
  buffer with brightness scaling. `ws2812_init` + `ws2812_show` bit-bang the strip (first cut).

### `ir_nec` — IR remote receive (NEC)

- `ir_nec_decode(us, count, &result)` (pure, host-tested): mark/space durations → address +
  command (command complement validated; `raw` for extended remotes). `ir_nec_rx_init/_read` capture + decode.

### `pir` — motion sensor

- `pir_init` + `pir_update(raw, now_ms)` (pure, host-tested): retrigger/hold FSM → PIR_MOTION_START/END.
  `pir_gpio_init/_poll` read the pin + run the FSM.

### `ssd1306` — 128×64 OLED (I2C)

- `ssd1306_fb_*` (pure, host-tested): 1bpp page framebuffer (set/get/fill/clear pixel).
  `ssd1306_init` + `ssd1306_show` drive the panel over `i2c_bus`.

### `max7219` — LED 7-seg / 8×8 matrix (SPI)

- `max7219_frame` + `max7219_digit_segments` (pure, host-tested). `max7219_init(self,
  cs_pin, intensity)` / `max7219_set_digit` / `max7219_set_row` — each instance owns a
  software CS GPIO (`spi_device`), so several MAX7219s coexist on the shared `spi_bus`.

### `rotary` — quadrature encoder

- `rotary_init` + `rotary_update` (pure, host-tested): table-driven quarter-step decode → CW/CCW + position.
  `rotary_gpio_init` / `rotary_gpio_poll` read the A/B pins.

### `bh1750` — ambient light (I2C)

- `bh1750_parse` (pure, host-tested): 16-bit raw count → milli-lux (lux = raw / 1.2).
  `bh1750_init` / `bh1750_read` over `i2c_bus` (continuous high-res mode, raw read).

### `sht3x` — temperature + humidity (I2C)

- `sht3x_parse` (pure, host-tested): validate 2× Sensirion CRC8 → milli-°C / milli-%RH
  (int64 widen-before-multiply). `sht3x_init` / `sht3x_read` over `i2c_bus`
  (single-shot high repeatability, ~15 ms).

### `ds3231` — real-time clock (I2C)

- `ds3231_decode` / `ds3231_encode` / `ds3231_temp_mc` (pure, host-tested): BCD↔time +
  on-chip temperature. `ds3231_init` / `ds3231_get` / `ds3231_set` / `ds3231_read_temp`
  over `i2c_bus` (register-mapped).

### `sk6812` — RGBW addressable LED

- `sk6812_render` (pure, host-tested): pack RGBW pixels into a brightness-scaled GRBW
  byte buffer. `sk6812_init` / `sk6812_show` bit-bang the strip (32 bits/pixel).

### `ads1115` — 16-bit 4-channel ADC (I2C)

- `ads1115_to_microvolts` + `ads1115_config` (pure, host-tested): PGA-scaled µV
  conversion (int64 widen) + single-shot config word. `ads1115_init` /
  `ads1115_read(channel)` over `i2c_bus`.

### `ina219` — current / power monitor (I2C)

- `ina219_bus_mv` / `ina219_shunt_uv` / `ina219_current_ua` / `ina219_power_uw` /
  `ina219_calibration` (pure, host-tested). `ina219_init(shunt, maxI)` +
  `ina219_read_*` over `i2c_bus`.

### Compile-gate

L2 wrappers require the real SDK (symbol resolution, FreeRTOS headers). They
are **not host-testable** until flashed on hardware. Correctness is verified by
the **compile-gate** project at `../esp-gate/`:

```sh
# Set SDK env first (once per shell):
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate

cd esp-gate && make defconfig && make -j4   # produces build/esp_libs_gate.elf
```

A successful link (`GATE_OK`) means all L2 symbols resolve against the SDK.
Hardware / runtime tests are out of scope for this layer.

## License

MIT — see the workspace `common/LICENSE`.
