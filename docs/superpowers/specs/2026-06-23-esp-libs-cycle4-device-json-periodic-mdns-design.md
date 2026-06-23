# esp-libs cycle 4 — Layer-2 `device_id` + `json` + `periodic` + `mdns_node` (design)

Date: 2026-06-23
Location: `clibs/esp-libs/` (the component) + `clibs/esp-gate/` (the compile-gate project), both committed directly in `clibs`.

## Context

`clibs` is **libraries only** — the smart-home node app (`node_core`) is built in a separate repo later
and *consumes* `esp-libs`. This cycle adds four more Layer-2 wrappers that complete the reusable
plumbing a node needs (after `wifi_sta` + `mqtt_node` + `nvs_kv`):

- **`device_id`** — derive a stable id string from the MAC (for MQTT `client_id`, default `node_id`, topic prefix).
- **`json`** — a no-malloc JSON *builder* for telemetry/state payloads + thin cJSON-based *getters* for inbound commands.
- **`periodic`** — wrap `esp_timer` as a periodic-callback helper (telemetry cadence).
- **`mdns_node`** — mDNS init/hostname + resolve a host to an IP (named `mdns_node` to avoid the SDK header `mdns.h`).

New this cycle: two wrappers have **host-testable pure logic** (`device_id_format`, the JSON builder),
so `make test` grows from 4 to 6 suites. The rest is SDK glue → compile-gate only, like cycles 2–3.

**Build env (Step-0 recipe, verified; legacy `make`, NOT idf.py):**
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig && make   # links build/esp_libs_gate.elf
```

## Decisions (confirmed with user)

| Decision | Choice |
|----------|--------|
| Scope | `device_id` + `json` + `periodic` + `mdns_node` (all four). OTA and `node_core` are out (OTA later; node_core is a separate app repo). |
| device_id | hex of the **last 3 MAC bytes** with a caller prefix (e.g. `"esp-3c71bf"`) — short + unique enough. |
| json | no-malloc builder (host-tested) for outbound; cJSON-based getters (compile-gate) for inbound. |
| periodic | wrap `esp_timer` (callback type already matches — direct passthrough). |
| mdns | named `mdns_node` (SDK header is `mdns.h`); init/hostname + resolve-A. |
| Test posture | host-Unity for the two pure parts (`device_id_format`, `json_build`); compile-gate for all SDK glue. |
| Out of scope | OTA, `node_core` app, hardware/runtime test, mDNS service advertisement beyond hostname. |

## Architecture

```
clibs/esp-libs/
├── Makefile               # CHANGED: +device_id +json_build host-test targets (test -> 6 suites)
├── component.mk           # CHANGED: +4 dirs in all 3 lists; maybe +COMPONENT_REQUIRES
├── backoff/ debounce/ filter/ mqtt_topic/   # L1 (unchanged)
├── wifi_sta/ mqtt_node/ nvs_kv/             # L2 cycles 2-3 (unchanged)
├── device_id/   device_id.h  device_id.c  device_id_get.c  test_device_id.c   # NEW
├── json/        json_build.{h,c}  json_get.{h,c}  test_json_build.c           # NEW
├── periodic/    periodic.{h,c}                                                # NEW
└── mdns_node/   mdns_node.{h,c}                                               # NEW
clibs/esp-gate/main/main.c   # CHANGED: touch the 4 wrappers' symbols
```

Each wrapper is independent. SDK components used: `esp8266` (`esp_read_mac`, `esp_timer`), `json`
(cJSON), `mdns`, `lwip` (`ip4addr_ntoa`). The make build exposes their includes globally; if a
link/include error arises, the implementer adds them to `COMPONENT_REQUIRES` — the gate is the check.

## `device_id` (`esp-libs/device_id/`)

Header `device_id.h` (guard `CLIBS_DEVICE_ID_H`, `<stdint.h>`, `<stddef.h>`, `esp_err.h`):
```c
/* Pure: format the LAST 3 bytes of `mac` as lowercase hex after `prefix`, into
 * buf[n] (NUL-terminated). e.g. prefix "esp-", mac {..,0x3c,0x71,0xbf} -> "esp-3c71bf".
 * prefix NULL -> no prefix. Returns the string length (excl NUL), or -1 on truncation /
 * NULL mac/buf / n==0. */
int device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix);

/* SDK glue: read the station MAC (esp_read_mac, ESP_MAC_WIFI_STA) and device_id_format it. */
esp_err_t device_id_get(char *buf, size_t n, const char *prefix);
```
- `device_id.c` = `device_id_format` only — pure, **no SDK include** (so the host test compiles it).
- `device_id_get.c` = `device_id_get` — includes `esp_system.h`; calls `esp_read_mac` then `device_id_format`.
- `test_device_id.c` (host): `device_id_format({0,1,2,3,4,5}, buf, n, "esp-")` -> `"esp-030405"` len 10;
  prefix NULL -> `"030405"`; truncation into a small buffer -> -1; NULL guards.

## `json` (`esp-libs/json/`)

### Builder — no-malloc, host-tested (`json_build.{h,c}`, guard `CLIBS_JSON_BUILD_H`)
```c
typedef struct {
    char  *buf;
    size_t cap;     /* buffer capacity */
    size_t len;     /* current length (excl NUL) */
    bool   err;     /* sticky overflow/arg error */
    bool   first;   /* no comma before the first member */
} json_build;

void json_build_init(json_build *b, char *buf, size_t cap);          /* writes "{", first=true */
void json_build_str(json_build *b, const char *key, const char *val);/* "key":"val" (escapes " and \) */
void json_build_int(json_build *b, const char *key, int32_t val);    /* "key":123 */
void json_build_raw(json_build *b, const char *key, const char *raw);/* "key":raw (caller-formed: bool, float-as-string, nested) */
int  json_build_end(json_build *b);   /* writes "}", NUL-terminates; returns len or -1 if err/overflow */
```
- Every `_str/_int/_raw` writes a leading `,` unless `first`, then `"key":`, then the value. On any
  overflow (would exceed `cap-1`) sets `err` and stops appending (sticky); `json_build_end` returns -1.
- Escaping in `_str`: only `"` -> `\"` and `\` -> `\\` (sufficient for node payloads; document that
  control chars are not escaped — callers send plain ASCII values).
- Pure C (`<stdint.h>`,`<stdbool.h>`,`<stddef.h>`,`<string.h>`), no malloc. Host-testable.
- `test_json_build.c`: build `{"state":"on","rssi":-60}` into a 64-byte buffer -> exact string, len
  matches; a `_raw` member (`"ok":true`); overflow into a tiny buffer -> `_end` returns -1, `err` set;
  empty object -> `"{}"`; escaping (`"a\"b"`) -> `"a\\\"b"`.

### Getters — cJSON, compile-gate (`json_get.{h,c}`, guard `CLIBS_JSON_GET_H`)
```c
/* Parse `json`, read string field `key` into buf[n] (NUL-term). Missing/not-a-string/parse-fail ->
 * copy `def` (NULL->"") , return 0; on success return the copied length. -1 on NULL buf / n==0.
 * (cJSON_Parse mallocs a tree and frees it before returning — fine for infrequent commands.) */
int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def);
/* Parse `json`, read integer field `key`. Missing/not-a-number/parse-fail -> def. */
int json_get_int(const char *json, const char *key, int def);
```
- `.c` includes `cJSON.h`; each call does `cJSON_Parse` -> `cJSON_GetObjectItem` -> type check
  (`cJSON_IsString` / number) -> copy `valuestring` / read `valueint` -> `cJSON_Delete`.

## `periodic` (`esp-libs/periodic/`, guard `CLIBS_PERIODIC_H`)
```c
#include "esp_timer.h"   /* for esp_timer_handle_t */
typedef struct { esp_timer_handle_t h; bool started; } periodic;
typedef void (*periodic_cb)(void *ctx);   /* == esp_timer_cb_t; runs in the esp_timer task — keep short */

esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx);
esp_err_t periodic_stop(periodic *self);
```
- `periodic_start`: NULL/`cb` guards; if already `started` -> `ESP_ERR_INVALID_STATE`. Build
  `esp_timer_create_args_t { .callback = cb, .arg = ctx, .name = "periodic" }` (callback type matches
  `periodic_cb` exactly — no trampoline), `esp_timer_create(&args, &self->h)`, then
  `esp_timer_start_periodic(self->h, (uint64_t)period_ms * 1000)`; set `started`.
- `periodic_stop`: if `started` -> `esp_timer_stop(self->h)` + `esp_timer_delete(self->h)`, clear
  `started`. Idempotent / NULL-safe. Compile-gate only.

## `mdns_node` (`esp-libs/mdns_node/`, guard `CLIBS_MDNS_NODE_H`)
```c
#include "esp_err.h"
esp_err_t mdns_node_init(const char *hostname);  /* mdns_init() + (hostname? mdns_hostname_set) */
/* Resolve `host` (A record) and write the dotted-quad into ip_buf[n]. timeout in ms. */
esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms);
```
- `mdns_node_init`: `mdns_init()`; if `hostname != NULL` -> `mdns_hostname_set(hostname)`.
  init-once is handled by the SDK (`mdns_init` is safe-ish; we just propagate its error).
- `mdns_node_resolve`: NULL/`n==0` guards; `ip4_addr_t addr; esp_err_t e = mdns_query_a(host,
  timeout_ms, &addr)`; on `ESP_OK` copy `ip4addr_ntoa(&addr)` into `ip_buf` bounded (`ip4addr_ntoa`
  returns a pointer to a static buffer — copy immediately); else return `e` and set `ip_buf[0]='\0'`.
- `.c` includes `mdns.h` (and `lwip/ip4_addr.h` for `ip4addr_ntoa`). Compile-gate only.

## Build infrastructure changes

### `esp-libs/component.mk`
Add the four dirs:
```makefile
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o
```
(`test_*.c` still excluded.) If the gate fails on unresolved includes/symbols, add
`COMPONENT_REQUIRES := nvs_flash mqtt mdns json esp_timer` (esp_timer/esp8266 are usually implicit).

### `esp-libs/Makefile`
Add two host-test targets so `make test` runs 6 suites:
- `device_id`: compile `device_id/test_device_id.c` + `device_id/device_id.c` + Unity, run.
- `json_build`: compile `json/test_json_build.c` + `json/json_build.c` + Unity, run.
- Add both to the `test:` aggregate and to the `strict:` gate (compile `device_id/device_id.c` and
  `json/json_build.c` with `-Werror`). The SDK-glue `.c` files are NOT in the host Makefile.

### `esp-gate/main/main.c`
Add includes + symbol-touch blocks (gate-only, never runs) for `device_id_get`, `json_build_*` +
`json_get_str`, `periodic_start/stop`, `mdns_node_init/resolve`.

## Testing
- **Host:** `cd esp-libs && make test` -> **6 suites** (backoff, debounce, filter, mqtt_topic,
  device_id, json_build) all `0 Failures`; `make strict` -> `strict: OK`.
- **ESP compile-gate:** `cd esp-gate && make defconfig && make` -> links `build/esp_libs_gate.elf`;
  `xtensa-lx106-elf-nm` shows the new symbols as `T`.
- No hardware / runtime tests.

## Conventions & scope
- Pure parts: freestanding C, no SDK include (so they host-compile); SDK-glue parts include SDK headers.
- Guards `CLIBS_*_H`; `esp_err_t` returns for SDK glue; `int` length/sentinel for pure formatters;
  NULL-arg guards; caller-provided transparent struct storage (no malloc) for `json_build`/`periodic`;
  diagnostics via `esp_log` in SDK glue (never `common/log`).
- Naming: `mdns_node` (avoid SDK `mdns.h`), consistent with `mqtt_node` (avoid SDK `mqtt_client.h`).
- Out of scope: OTA, `node_core`, hardware test, mDNS service records beyond hostname.

## Build sequence
1. **`device_id`**: pure `device_id_format` (+ host test, RED->GREEN) + `device_id_get` SDK glue;
   wire `component.mk` + `Makefile` (device_id suite) + `esp-gate/main.c`; host `make test` (5 suites
   now) + esp-gate links.
2. **`json`**: `json_build` (+ host test) + `json_get` cJSON getters; wire all three; host `make test`
   (6 suites) + esp-gate links.
3. **`periodic` + `mdns_node`** (both compile-gate): wrappers + wire `component.mk` + `esp-gate/main.c`;
   esp-gate links; host tests still 6 suites green.
4. **Final**: `make test`/`strict` (6 suites) + esp-gate link + symbol check; README note (4 new L2 libs).
5. (clibs feature branch -> merge -> push.)
