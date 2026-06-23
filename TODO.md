# clibs — TODO (việc làm sau)

`clibs` = **thư viện C dùng chung cho IoT** (libraries only). Ứng dụng (node app) build ở **repo riêng**.
Hiện trạng: `esp-libs` có L1 (pure, host-test) + L2 (SDK wrappers) + L3 (device drivers). `make test` = 17 host suites; ESP8266 compile-gate (`esp-gate`) link sạch. **Chưa chạy thật trên phần cứng.**

Roadmap pre-hardware **A→D→C→B đã xong hết** (xem lịch sử cycle trong git + `docs/superpowers/`).

---

## 1. Thêm thư viện cho esp-libs (library-only, không cần phần cứng)

- [ ] **bme280** — driver cảm biến T/H/P qua I2C (chạy trên `i2c_bus` của cycle 7). Cycle riêng. Công thức bù trừ Bosch (fixed-point) **host-test được** với calib+raw→expected. Là mốc "driver L3 đầu tiên trên bus L2".
- [ ] **OTA** — cập nhật firmware qua HTTP từ gateway (`esp_https_ota` / `esp_ota_ops`): download + ghi partition + verify + reboot. Món **lớn**, cycle riêng.
- [ ] Thêm L3 khi cần: `ws2812` (RGB, timing-critical bit-bang), `pir` (digital như button), IR remote (decode NEC — pure host-test), stepper, DS18B20 multi-drop (ROM search).
- [ ] Optional L2: `watchdog` (esp_task_wdt), `deep_sleep` (node chạy pin), `http_client`.

## 2. Kiểm thử PHẦN CỨNG (khi có ESP8266 thật) — bucket lớn nhất đang hoãn

Chạy 1 lượt trên chip thật + fix timing/protocol cho phần glue (compile-gate mới chỉ chứng minh build+link):
- [ ] `dht` (cycle 5) — bit-bang timing DHT22/11.
- [ ] `ds18b20` (cycle 8) — 1-Wire timing; **đổi `ets_delay_us(750000)` busy-wait → `vTaskDelay`** cho lần convert 750ms.
- [ ] `hcsr04` (cycle 8) — trigger/echo timing.
- [ ] `relay` / `button` / `pwm_dimmer` / `servo` — GPIO + PWM output thật.
- [ ] `i2c_bus` / `spi_bus` (cycle 7) — protocol/ACK/timing với chip thật.
- [ ] `adc_a0` (cycle 7) — scaling A0 (0–1V vs divider trên NodeMCU).

## 3. node_core app — REPO RIÊNG (KHÔNG phải clibs)

- [ ] Tạo repo app mới *consume* esp-libs (symlink/drop `esp-libs/` vào `components/`).
- [ ] Wire: `wifi_sta` → `mqtt_node` (presence/telemetry) → `nvs_kv` provisioning → sensors/actuators; dùng `hysteresis` (thermostat), `throttle` (rate-limit publish), `device_id` (client_id/topic).
- [ ] Typed `node_config` struct (trên `nvs_kv`): wifi ssid/pass, mqtt uri, node_id, room.
- [ ] **Khi tạo repo mới dưới ~/git: thêm `includeIf` git-identity (mannk) hoặc nó commit bằng email công ty** (xem reference git-identity).

## 4. CI / chất lượng (follow-up của D)

- [ ] ESP cross-compile gate trong CI (Linux xtensa toolchain + SDK) — enhancement, optional (hiện CI chỉ chạy host test trên gcc).
- [ ] Fuzz các parser thuần: `dht_parse`, `json_build`/`json_get`, `mqtt_topic_match`, `ds18b20_parse` (libFuzzer).
- [ ] Coverage (gcov/lcov) + static analysis (cppcheck/clang-tidy) trên các lib thuần.
- [ ] `gh` chưa auth ở máy local → không poll được CI run; check tab Actions trên GitHub.

## 5. avr-libs track (đã hoãn từ lâu)

- [ ] Register-driver test cycle (cần `simavr`); dedup `ringbuff`/pin-mapping giữa các module.

## 6. Minor đã hoãn (cosmetic, từ các review)

- [ ] cycle 4: `device_id_get` double-clear buf; thiếu test exact-fit `device_id_format`; `periodic_stop` bỏ qua rc của `esp_timer_stop`.
- [ ] cycle 5: `dht.h` thừa `<stdint.h>`; `dht_init` bỏ qua rc của `gpio_set_direction/level`.
- [ ] Workflow lesson: khi dùng parallel Workflow, giới hạn reviewer per-lib CHỈ soi file của lib đó (không soi integration wiring) để tránh false-positive `needs_changes` (xảy ra ở cycle 8).
