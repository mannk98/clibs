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

## License

MIT — see the workspace `common/LICENSE`.
