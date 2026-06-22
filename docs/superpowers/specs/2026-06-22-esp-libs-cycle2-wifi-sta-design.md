# esp-libs cycle 2 — Layer-2 infra + wifi_sta (design)

Date: 2026-06-22
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project), both committed directly in `clibs`.

## Context

esp-libs Layer 1 (pure libs: backoff/debounce/filter/mqtt_topic) is done + host-Unity-tested.
Layer 2 = thin ESP8266-RTOS-SDK wrappers, which `#include` SDK headers → **not host-testable**
(compile-gate only). This cycle delivers the **build infrastructure** to compile esp-libs as an
ESP8266 component + a `make` compile-gate project, plus the first wrapper **`wifi_sta`**
(STA connect + reconnect spaced by our `backoff`). No hardware; verification = it cross-compiles
+ links for the ESP8266 target.

**Build env (Step-0 recipe, verified working on this macOS arm64; legacy `make`, NOT idf.py):**
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
```

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | Layer-2 infra (component + gate project) + `wifi_sta` only; mqtt/nvs_config next cycles |
| esp-libs as component | One ESP8266 component `esp_libs` via `esp-libs/component.mk` (lib `.o`s only; `test_*.c` excluded) |
| Compile-gate | New `clibs/esp-gate/` minimal `make` project; `components/esp_libs` is a **symlink** to `../../esp-libs`; gate = `make defconfig && make` links |
| L1 host tests | Unchanged (`esp-libs/Makefile` → `make test`/`strict` with host `cc`) |
| wifi_sta test | **Compile-gate only** (SDK glue; no host test, no hardware). Retry-spacing logic = `backoff` (already host-tested). |
| Out of scope | `mqtt`, `nvs_config` wrappers; AP provisioning; any runtime/hardware test |

## Architecture

esp-libs now builds **two ways**, both from the same sources:
- **Host (L1 logic):** `cc` + Unity via `esp-libs/Makefile` (`make test`/`strict`) — unchanged.
- **Target (compile-gate, incl. L2):** `xtensa-lx106-elf-gcc` as the `esp_libs` component, built by the `esp-gate` project.

```
clibs/
├── esp-libs/
│   ├── Makefile            # host tests (unchanged)
│   ├── component.mk        # NEW: esp-libs as ESP8266 component "esp_libs"
│   ├── backoff/ debounce/ filter/ mqtt_topic/   # L1 (unchanged)
│   └── wifi_sta/ wifi_sta.{h,c}                  # NEW (L2)
└── esp-gate/               # NEW: compile-gate make project
    ├── Makefile            # PROJECT_NAME + EXTRA_COMPONENT_DIRS=components
    ├── sdkconfig.defaults  # minimal (optional)
    ├── .gitignore          # build/  sdkconfig  sdkconfig.old
    ├── main/ main.c  component.mk
    └── components/esp_libs -> ../../esp-libs   # symlink
```

### `esp-libs/component.mk`
```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o
```
(The make build exposes all components' public include dirs globally, so `wifi_sta.c` can include `esp_wifi.h`/`tcpip_adapter.h`/`esp_event.h` without an explicit REQUIRES. If a link/include issue arises, the implementer adds `COMPONENT_REQUIRES`/`PRIV_REQUIRES` — the gate build is the check.)

### `esp-gate/Makefile`
```makefile
PROJECT_NAME := esp_libs_gate
EXTRA_COMPONENT_DIRS := $(abspath $(CURDIR)/components)
include $(IDF_PATH)/make/project.mk
```
`esp-gate/components/esp_libs` is a symlink to `../../esp-libs`, giving a clean one-component search dir (avoids scanning esp-libs' subdirs as stray components). `main/main.c`'s `app_main` references each lib symbol so the linker pulls the whole component (the gate). `make defconfig && make` linking = pass.

## `wifi_sta` wrapper (`esp-libs/wifi_sta/wifi_sta.{h,c}`)

STA-mode manager: connect to the gateway AP, and on disconnect reconnect **spaced by `backoff`**
(the SDK station example calls `esp_wifi_connect()` immediately on disconnect — we add backoff
spacing). Header guard `CLIBS_WIFI_STA_H`.
```c
typedef enum { WIFI_STA_DISCONNECTED, WIFI_STA_CONNECTING, WIFI_STA_CONNECTED } wifi_sta_state;
typedef void (*wifi_sta_cb)(wifi_sta_state state, void *ctx);
typedef struct {
    const char  *ssid;
    const char  *password;
    uint32_t     backoff_base_ms;   /* reconnect backoff base (e.g. 500) */
    uint32_t     backoff_max_ms;    /* reconnect backoff cap  (e.g. 30000) */
    wifi_sta_cb  cb;                 /* optional status callback (may be NULL) */
    void        *cb_ctx;
} wifi_sta_config;

esp_err_t      wifi_sta_start(const wifi_sta_config *cfg);
esp_err_t      wifi_sta_stop(void);
wifi_sta_state wifi_sta_get_state(void);
```
**Internals:** `tcpip_adapter_init()` → `esp_event_loop_create_default()` → `esp_wifi_init()` →
register `WIFI_EVENT` + `IP_EVENT` handlers → `esp_wifi_set_mode(WIFI_MODE_STA)` →
`esp_wifi_set_config(ESP_IF_WIFI_STA, …)` → `esp_wifi_start()`. A dedicated FreeRTOS
**reconnect task** owns a `backoff` instance. Event handler (must not block):
- `WIFI_EVENT_STA_START` → `esp_wifi_connect()`, state CONNECTING.
- `WIFI_EVENT_STA_DISCONNECTED` → state DISCONNECTED, fire cb, notify the reconnect task.
- `IP_EVENT_STA_GOT_IP` → state CONNECTED, `backoff_reset()`, fire cb.

Reconnect task loop: block on a task notification; on signal, `vTaskDelay(backoff_next())` then
state CONNECTING + `esp_wifi_connect()`. (Spacing lives in the task, off the event-loop thread.)

The exact SDK symbol names/signatures are taken from `examples/wifi/getting_started/station`
and the SDK headers; the implementer adjusts to whatever compiles+links in `esp-gate`.

## Testing
- L1: host-Unity unchanged (`make test`/`strict`).
- Infra + `wifi_sta`: **compile-gate** — `cd esp-gate && make defconfig && make` produces
  `esp_libs_gate.elf` (links → pass). No host test of SDK glue, no hardware. The only pure logic
  (retry spacing) is `backoff`, already host-tested.

## Conventions & scope
- Wrapper header may include SDK headers (component-only, not host-compilable — expected for L2).
- `CLIBS_WIFI_STA_H` guard; `esp_err_t` returns; NULL-config guard returns an error.
- Wrapper uses `esp_log` (`ESP_LOGI`/`W`/`E`) for diagnostics (not `common/log`).
- Out of scope: mqtt/nvs_config wrappers, provisioning, runtime/hardware verification.

## Build sequence
1. Component infra: `esp-libs/component.mk` + `esp-gate/` (Makefile, main, symlink, .gitignore) + a trivial `wifi_sta` stub so the gate links — establish `cd esp-gate && make` GREEN.
2. `wifi_sta` real implementation (STA connect + reconnect task + backoff) → gate still links GREEN.
3. Final: `make test`/`strict` (L1 still green) + `esp-gate` gate green; esp-libs README note (Layer-2 / esp-gate).
4. (clibs feature branch → merge → push.)

(Steps 1+2 may fold into one wifi_sta task; the gate-link is the GREEN for both.)
