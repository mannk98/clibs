# esp-libs

Reusable C building blocks for **ESP8266** smart-home nodes (ESP8266 RTOS SDK,
FreeRTOS). Built bottom-up:

- **Layer 1 — pure logic (this set):** plain portable C, host-unit-tested with
  Unity, no SDK/hardware needed (compiles on ESP8266 unchanged).
- *Layer 2 — SDK wrappers* (wifi / mqtt-client / nvs) — later, needs the SDK.
- *Layer 3 — device drivers* (DHT, DS18B20, relay…) — later, needs hardware.

## Layer-1 libraries

| Lib | What it does |
|-----|--------------|
| `backoff` | Exponential reconnect backoff for WiFi/MQTT (`backoff_init/next/reset`). |
| `debounce` | Input debounce with edge detection (`debounce_init/update/level`). |
| `filter` | Integer EMA smoothing for ADC (`ema_init/update`, alpha = 1/2^shift, no floats). |
| `mqtt_topic` | Topic join + MQTT `+`/`#` wildcard match (`mqtt_topic_join/match`). |

Each is freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`, +`<string.h>` in
`mqtt_topic`), static / no-malloc, transparent struct + `self`-methods.

## Tests

Host unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity), reused from
`../common/third_party/unity`.

```sh
make test     # build + run all four suites
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

### Compile-gate

L2 wrappers require the real SDK (symbol resolution, FreeRTOS headers). They
are **not host-testable** until flashed on hardware. Correctness is verified by
the **compile-gate** project at `../esp-gate/`:

```sh
# Set SDK env first (once per shell):
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate

cd esp-gate && make -j4   # produces build/esp_libs_gate.elf
```

A successful link (`GATE_OK`) means all L2 symbols resolve against the SDK.
Hardware / runtime tests are out of scope for this layer.

## License

MIT — see the workspace `common/LICENSE`.
