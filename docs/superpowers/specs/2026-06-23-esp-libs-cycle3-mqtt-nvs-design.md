# esp-libs cycle 3 — Layer-2 `mqtt_client` + `nvs_kv` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project), both committed directly in `clibs`.

## Context

esp-libs Layer 1 (pure libs) is done + host-Unity-tested. Cycle 2 delivered the ESP8266
build infrastructure (`esp-libs/component.mk` → component `esp_libs`; `esp-gate/` → `make`
compile-gate project with a symlinked component) and the first Layer-2 wrapper `wifi_sta`.

This cycle adds two more Layer-2 wrappers to the **same** `esp_libs` component:
- **`mqtt_client`** — singleton MQTT manager (connect to the gateway broker, publish/subscribe,
  presence via Last-Will), built on the SDK's `esp-mqtt` component.
- **`nvs_kv`** — reusable key/value wrapper over one NVS namespace (hides the verbose
  open/probe-size/commit/close churn of the SDK `nvs` API).

Both are thin ESP8266-RTOS-SDK wrappers that `#include` SDK headers → **not host-testable**
(compile-gate only), exactly like `wifi_sta`. No hardware; verification = they cross-compile
+ link for the ESP8266 target.

**Build env (Step-0 recipe, verified working on this macOS arm64; legacy `make`, NOT idf.py):**
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig && make   # links build/esp_libs_gate.elf
```

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | `mqtt_client` + `nvs_kv` only; `node_core` app next cycle |
| `nvs_kv` level | **Generic reusable key/value helper** (schema-agnostic). The node-typed config struct (wifi/mqtt/node_id/room) is deferred to `node_core`. |
| `mqtt_client` presence | **Include LWT presence** — optional `presence_topic` + retained `online_msg` on connect + `offline_msg` as the Last-Will. |
| MQTT reconnect | Rely on **esp-mqtt's built-in auto-reconnect** (`disable_auto_reconnect=false` default). No custom backoff (unlike `wifi_sta`, whose SDK path reconnects immediately). |
| Wrapper shape | Singleton managers mirroring `wifi_sta` (state enum + callback + config struct + start/stop/get_state). One node = one broker connection. |
| Test | **Compile-gate only** for both (SDK glue; no host test, no hardware). No new host-testable pure logic is introduced. |
| Out of scope | `node_core` app, typed `node_config` struct, TLS (LAN broker), AP provisioning, any runtime/hardware test |

## Architecture

```
clibs/esp-libs/
├── component.mk            # CHANGED: +mqtt_client +nvs_kv in all 3 lists
├── backoff/ debounce/ filter/ mqtt_topic/      # L1 (unchanged)
├── wifi_sta/  wifi_sta.{h,c}                    # L2 (unchanged)
├── mqtt_client/ mqtt_client.{h,c}              # NEW (L2)
└── nvs_kv/      nvs_kv.{h,c}                    # NEW (L2)
clibs/esp-gate/main/main.c   # CHANGED: touch mqtt_client + nvs_kv symbols
```

`mqtt_client` and `nvs_kv` are independent of each other and of `wifi_sta` (no link-time
dependency between wrappers). esp-mqtt and nvs_flash are SDK components; the make build exposes
their public include dirs globally (like `esp_wifi` for `wifi_sta`), so the wrappers can include
`mqtt_client.h` / `nvs.h` / `nvs_flash.h` without an explicit REQUIRES. If a link/include error
arises, the implementer adds `COMPONENT_REQUIRES`/`PRIV_REQUIRES` to `component.mk` — the gate
build is the check.

## `mqtt_client` wrapper (`esp-libs/mqtt_client/mqtt_client.{h,c}`)

Singleton MQTT manager. Header guard `CLIBS_MQTT_CLIENT_H`. Includes `esp_err.h` (+ SDK
`mqtt_client.h` in the `.c`).

```c
typedef enum { MQTT_CLI_DISCONNECTED, MQTT_CLI_CONNECTED } mqtt_cli_state;
typedef void (*mqtt_cli_state_cb)(mqtt_cli_state state, void *ctx);
/* inbound message; topic/data are NOT NUL-terminated — use the given lengths. */
typedef void (*mqtt_cli_msg_cb)(const char *topic, int topic_len,
                                const char *data, int data_len, void *ctx);
typedef struct {
    const char *uri;             /* "mqtt://192.168.4.1:1883" (required) */
    const char *client_id;       /* NULL → esp-mqtt default */
    const char *username;        /* NULL = no auth */
    const char *password;        /* NULL = no auth */
    int         keepalive;       /* 0 → SDK default (120 s) */
    /* presence (LWT) — leave presence_topic NULL to disable all of this block */
    const char *presence_topic;  /* e.g. "home/<room>/<node>/online" */
    const char *online_msg;      /* RETAINED publish on connect (e.g. "1") */
    const char *offline_msg;     /* Last-Will message (e.g. "0") */
    int         presence_qos;
    mqtt_cli_state_cb state_cb;  /* optional (may be NULL) */
    mqtt_cli_msg_cb   msg_cb;    /* optional (may be NULL) */
    void             *cb_ctx;
} mqtt_cli_config;

esp_err_t      mqtt_client_start(const mqtt_cli_config *cfg);
esp_err_t      mqtt_client_stop(void);
/* len 0 → esp-mqtt computes strlen(data). Returns msg_id (>=0) or -1 on failure. */
int            mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);
int            mqtt_client_subscribe(const char *topic, int qos);
mqtt_cli_state mqtt_client_get_state(void);
```

**Internals:**
- `mqtt_client_start`: NULL-guard cfg/cfg->uri. Build `esp_mqtt_client_config_t`:
  `uri`, `client_id`, `username`, `password`, `keepalive` (only set if non-zero). If
  `presence_topic != NULL`: `lwt_topic = presence_topic`, `lwt_msg = offline_msg`,
  `lwt_retain = 1`, `lwt_qos = presence_qos`. Then `esp_mqtt_client_init(&c)` →
  `esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, handler, NULL)` →
  `esp_mqtt_client_start(client)`. **Idempotent**: a `static bool s_inited` init-once guard
  protects `esp_mqtt_client_init` (a second `start` returns `ESP_OK` without re-init), mirroring
  `wifi_sta`. The single client handle is held in a static.
- Event handler (`MQTT_EVENT_*`, must not block):
  - `MQTT_EVENT_CONNECTED` → state `CONNECTED`, fire `state_cb`; if presence configured →
    `esp_mqtt_client_publish(client, presence_topic, online_msg, 0, presence_qos, 1)` (retained).
  - `MQTT_EVENT_DISCONNECTED` → state `DISCONNECTED`, fire `state_cb`.
  - `MQTT_EVENT_DATA` → fire `msg_cb(topic, topic_len, data, data_len, ctx)` from the event.
- `mqtt_client_publish` / `_subscribe`: thin pass-through to `esp_mqtt_client_publish` /
  `esp_mqtt_client_subscribe` on the held handle; return -1 if not started.
- `mqtt_client_stop`: `esp_mqtt_client_stop(client)` + state `DISCONNECTED` (keeps the handle
  for a later restart, same init-once philosophy as `wifi_sta`).
- Diagnostics via `esp_log` (`ESP_LOGI/W/E`), not `common/log`.

The exact SDK symbol names/signatures come from `components/mqtt/esp-mqtt/include/mqtt_client.h`
and the `examples/mqtt` example; the implementer adjusts to whatever compiles+links in `esp-gate`.

## `nvs_kv` wrapper (`esp-libs/nvs_kv/nvs_kv.{h,c}`)

Reusable key/value store over **one** NVS namespace. Caller-provided `nvs_kv` storage (no malloc),
transparent struct + `self`-methods (clibs convention). Header guard `CLIBS_NVS_KV_H`. Includes
`esp_err.h`, `<stdint.h>`, `<stdbool.h>`, `<stddef.h>` (+ SDK `nvs.h`/`nvs_flash.h` in the `.c`;
`nvs_handle_t` in the struct needs `nvs.h` in the header too).

```c
#include "nvs.h"   /* for nvs_handle_t */
typedef struct { nvs_handle_t h; bool is_open; } nvs_kv;

/* nvs_flash_init once; on NO_FREE_PAGES / new-version → nvs_flash_erase + retry. Init-once guard. */
esp_err_t nvs_kv_init(void);
esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable); /* NVS_READWRITE / NVS_READONLY */
void      nvs_kv_close(nvs_kv *self);
/* missing key → copy `def` into buf (bounded, NUL-term), return ESP_OK. Stored value longer than
 * n → ESP_ERR_NVS_INVALID_LENGTH (buf left as empty string). NULL guards → ESP_ERR_INVALID_ARG. */
esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def);
esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val);
/* missing key → *out = def, return ESP_OK. */
esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def);
esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val);
esp_err_t nvs_kv_commit(nvs_kv *self);   /* flush pending writes */
```

**Internals:**
- `nvs_kv_init`: `esp_err_t e = nvs_flash_init();` if `e == ESP_ERR_NVS_NO_FREE_PAGES`
  (or `ESP_ERR_NVS_NEW_VERSION_FOUND` if present in this SDK) → `nvs_flash_erase()` then
  `nvs_flash_init()` again. `static bool s_inited` guard so repeat calls are no-ops returning
  `ESP_OK`.
- `nvs_kv_open`: `nvs_open(ns, writable ? NVS_READWRITE : NVS_READONLY, &self->h)`; set
  `is_open` on success. NULL-guard self/ns.
- `nvs_kv_get_str`: NULL-guard; if `!is_open` → `ESP_ERR_INVALID_STATE`. Probe size with
  `nvs_get_str(h, key, NULL, &len)`. If `ESP_ERR_NVS_NOT_FOUND` → copy `def` into `buf` (bounded
  by `n`, NUL-terminated; `def` NULL → empty string), return `ESP_OK`. If `len > n` →
  `buf[0]='\0'`, return `ESP_ERR_NVS_INVALID_LENGTH`. Else `nvs_get_str(h, key, buf, &n)`.
- `nvs_kv_get_u32`: `nvs_get_u32(h, key, out)`; on `ESP_ERR_NVS_NOT_FOUND` → `*out = def`, `ESP_OK`.
- `set_str`/`set_u32`/`commit`: pass-through to `nvs_set_str`/`nvs_set_u32`/`nvs_commit` on `self->h`
  (each with `is_open` + NULL guards).
- `nvs_kv_close`: `nvs_close(self->h)` if open; clear `is_open`.
- Diagnostics via `esp_log`.

## Build infrastructure changes

### `esp-libs/component.mk`
Add the two dirs to each list:
```makefile
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta mqtt_client nvs_kv
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta mqtt_client nvs_kv
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  mqtt_client/mqtt_client.o nvs_kv/nvs_kv.o
```
(`test_*.c` remain excluded — explicit `COMPONENT_OBJS`.)

### `esp-gate/main/main.c`
Add symbol-touch calls in `app_main` so the linker pulls the new objects (gate-only; never runs):
e.g. reference `mqtt_client_get_state()` and `nvs_kv_init()` behind the existing
`if`-that-never-fires pattern.

## Testing

- **L1:** host-Unity unchanged (`cd esp-libs && make test` → 4 suites, all 0 Failures; `make strict` → OK).
- **L2 (`mqtt_client` + `nvs_kv`):** **compile-gate** — `cd esp-gate && make defconfig && make`
  produces `build/esp_libs_gate.elf` (links → pass). No host test of SDK glue, no hardware.
- No new host-testable pure logic is introduced this cycle (topic building is already covered by
  the L1 `mqtt_topic` suite; presence-topic assembly is the caller's job).

## Conventions & scope

- Wrapper headers may include SDK headers (`esp_err.h`; `nvs_kv.h` also includes `nvs.h` for the
  handle type) — component-only, not host-compilable, expected for L2.
- `CLIBS_MQTT_CLIENT_H` / `CLIBS_NVS_KV_H` guards; `esp_err_t` returns where applicable; NULL-arg
  guards; init-once (`s_inited`) for both `mqtt_client_start` and `nvs_kv_init`.
- Singleton managers (one static client / init-once flash) mirroring `wifi_sta`.
- `nvs_kv` uses caller-provided transparent `nvs_kv` storage (no malloc).
- Wrappers use `esp_log`, not `common/log`.
- Out of scope: `node_core` app, typed `node_config` struct, TLS, provisioning, runtime/hardware verification.

## Build sequence

1. **`nvs_kv`** wrapper (`nvs_kv.{h,c}`) + wire into `component.mk` + `esp-gate/main.c` symbol-touch
   → `cd esp-gate && make` links GREEN.
2. **`mqtt_client`** wrapper (`mqtt_client.{h,c}`, incl. LWT presence) + wire into `component.mk` +
   `esp-gate/main.c` symbol-touch → gate links GREEN.
3. **Final:** `make test`/`strict` (L1 still green) + `esp-gate` gate green; esp-libs README note
   (mqtt_client + nvs_kv added to Layer-2).
4. (clibs feature branch → merge to clibs main → push.)

(Each wrapper task's GREEN is the gate-link; the two wrappers are independent and could be done in
either order — `nvs_kv` first as the simpler one.)
