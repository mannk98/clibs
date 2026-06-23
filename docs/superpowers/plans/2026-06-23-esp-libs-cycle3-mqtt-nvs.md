# esp-libs Cycle 3 — Layer-2 `mqtt_node` + `nvs_kv` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add two Layer-2 ESP8266-RTOS-SDK wrappers to the `esp_libs` component — `mqtt_node` (singleton MQTT manager with Last-Will presence, built on esp-mqtt) and `nvs_kv` (reusable key/value store over one NVS namespace) — verified by the `esp-gate` compile-gate.

**Architecture:** Both wrappers are thin SDK glue (`#include` SDK headers), so they are **compile-gate only** — no host unit test, no hardware, exactly like the existing `wifi_sta`. Each wrapper is one directory under `esp-libs/`, added to the single `esp_libs` component via `component.mk`, and exercised by symbol-touches in `esp-gate/main/main.c`. The per-task GREEN is: the `esp-gate` project cross-compiles and links `build/esp_libs_gate.elf`.

**Tech Stack:** ESP8266_RTOS_SDK v3.4 (xtensa-lx106-elf gcc 8.4.0 via Rosetta), legacy `make` build, SDK components `mqtt` (esp-mqtt), `nvs_flash`, `esp_event`, `esp_log`.

## Global Constraints

- **Build env (run once per shell before any gate build):**
  ```sh
  export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
  export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
  source ~/esp/idf-venv/bin/activate
  ```
- **Compile-gate posture:** `mqtt_node` and `nvs_kv` are SDK glue → **NOT host-testable**. Do NOT write host Unity tests for them and do NOT add a fake/empty test. The verification for each task is the `esp-gate` link. (This matches `wifi_sta` and the cycle-3 spec.)
- **Gate command** (from `clibs/esp-gate/`, after the env above): `make defconfig && make -j4` → must produce `build/esp_libs_gate.elf`. A successful link = the task's GREEN.
- **L1 host tests stay green:** `cd esp-libs && make test` (4 suites, all `0 Failures`) + `make strict` (`strict: OK`). Do not touch L1 sources, `common/`, `avr-libs/`, or `esp-idf-template/`.
- **Wrapper name `mqtt_node` (NOT `mqtt_client`):** the SDK's esp-mqtt public header is itself named `mqtt_client.h`. A wrapper header/dir named `mqtt_client` would be on the component include path and shadow the SDK header (`#include "mqtt_client.h"` would resolve to our own file), breaking symbol resolution. The wrapper is therefore named `mqtt_node` (dir, files, header guard, types, functions all `mqtt_node*`); its `.c` includes the SDK's `mqtt_client.h` unambiguously.
- **clibs conventions:** header guards `CLIBS_<NAME>_H`; `esp_err_t` returns where applicable; NULL-arg guards; init-once via `static bool s_inited`; singleton managers (one static client / one init-once flash); caller-provided transparent struct storage, no malloc; diagnostics via `esp_log` (`ESP_LOGI/W/E`), never `common/log`.
- **`component.mk` excludes `test_*.c`:** keep the explicit `COMPONENT_OBJS` listing only library `.o`s.
- **Commits:** end each commit message with the trailer `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `nvs_kv` wrapper + wire into component & gate

**Files:**
- Create: `esp-libs/nvs_kv/nvs_kv.h`
- Create: `esp-libs/nvs_kv/nvs_kv.c`
- Modify: `esp-libs/component.mk` (add `nvs_kv` to all three lists)
- Modify: `esp-gate/main/main.c` (touch `nvs_kv` symbols)
- Test: none (compile-gate only — see Global Constraints). GREEN = `esp-gate` links.

**Interfaces:**
- Consumes: SDK `nvs.h` (`nvs_handle_t`, `nvs_open`, `nvs_get_str/u32`, `nvs_set_str/u32`, `nvs_commit`, `nvs_close`, `NVS_READWRITE`/`NVS_READONLY`, `ESP_ERR_NVS_*`), `nvs_flash.h` (`nvs_flash_init`, `nvs_flash_erase`), `esp_err.h`, `esp_log.h`.
- Produces (later tasks / node_core rely on these exact signatures):
  ```c
  typedef struct { nvs_handle_t h; bool is_open; } nvs_kv;
  esp_err_t nvs_kv_init(void);
  esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable);
  void      nvs_kv_close(nvs_kv *self);
  esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def);
  esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val);
  esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def);
  esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val);
  esp_err_t nvs_kv_commit(nvs_kv *self);
  ```

- [ ] **Step 1: Write `esp-libs/nvs_kv/nvs_kv.h`**

```c
#ifndef CLIBS_NVS_KV_H
#define CLIBS_NVS_KV_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "nvs.h"   /* for nvs_handle_t */

/* Reusable key/value store over ONE NVS namespace. Caller owns the `nvs_kv`
 * storage (no malloc). Transparent struct + self-methods (clibs convention). */
typedef struct {
    nvs_handle_t h;
    bool         is_open;
} nvs_kv;

/* Initialise the NVS flash partition once (idempotent). On NO_FREE_PAGES /
 * NEW_VERSION_FOUND it erases the partition and retries. Returns ESP_OK on
 * success. Call once at boot before nvs_kv_open(). */
esp_err_t nvs_kv_init(void);

/* Open a namespace. writable=true -> NVS_READWRITE, false -> NVS_READONLY.
 * NULL self/ns -> ESP_ERR_INVALID_ARG. */
esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable);

/* Close the namespace handle (no-op if not open / self NULL). */
void nvs_kv_close(nvs_kv *self);

/* Read a string into buf[n] (always NUL-terminated on ESP_OK). Missing key ->
 * copies `def` (NULL treated as "") into buf and returns ESP_OK. A stored value
 * that would not fit in n -> buf set to "" and ESP_ERR_NVS_INVALID_LENGTH.
 * NULL self/key/buf or n==0 -> ESP_ERR_INVALID_ARG. Not open -> ESP_ERR_INVALID_STATE. */
esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def);

/* Write a string value (pending until nvs_kv_commit). */
esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val);

/* Read a u32. Missing key -> *out = def, returns ESP_OK. */
esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def);

/* Write a u32 value (pending until nvs_kv_commit). */
esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val);

/* Flush pending writes to flash. */
esp_err_t nvs_kv_commit(nvs_kv *self);

#endif /* CLIBS_NVS_KV_H */
```

- [ ] **Step 2: Write `esp-libs/nvs_kv/nvs_kv.c`**

```c
#include "nvs_kv.h"

#include <string.h>

#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "nvs_kv";
static bool s_inited;

esp_err_t nvs_kv_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs erase + reinit (err=0x%x)", err);
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err == ESP_OK) {
        s_inited = true;
    } else {
        ESP_LOGE(TAG, "nvs_flash_init failed: 0x%x", err);
    }
    return err;
}

esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable)
{
    if (self == NULL || ns == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->is_open = false;
    esp_err_t err = nvs_open(ns, writable ? NVS_READWRITE : NVS_READONLY, &self->h);
    if (err == ESP_OK) {
        self->is_open = true;
    }
    return err;
}

void nvs_kv_close(nvs_kv *self)
{
    if (self != NULL && self->is_open) {
        nvs_close(self->h);
        self->is_open = false;
    }
}

/* Copy `def` (NULL -> "") into buf[n]; INVALID_LENGTH if it would not fit. */
static esp_err_t copy_default(char *buf, size_t n, const char *def)
{
    if (def == NULL) {
        buf[0] = '\0';
        return ESP_OK;
    }
    size_t len = strlen(def);
    if (len >= n) {
        buf[0] = '\0';
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    memcpy(buf, def, len + 1);
    return ESP_OK;
}

esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def)
{
    if (self == NULL || key == NULL || buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t required = 0;
    esp_err_t err = nvs_get_str(self->h, key, NULL, &required);  /* probe size (incl NUL) */
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return copy_default(buf, n, def);
    }
    if (err != ESP_OK) {
        buf[0] = '\0';
        return err;
    }
    if (required > n) {
        buf[0] = '\0';
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    size_t cap = n;
    return nvs_get_str(self->h, key, buf, &cap);
}

esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val)
{
    if (self == NULL || key == NULL || val == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_set_str(self->h, key, val);
}

esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def)
{
    if (self == NULL || key == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = nvs_get_u32(self->h, key, out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = def;
        return ESP_OK;
    }
    return err;
}

esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val)
{
    if (self == NULL || key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_set_u32(self->h, key, val);
}

esp_err_t nvs_kv_commit(nvs_kv *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_commit(self->h);
}
```

- [ ] **Step 3: Wire `nvs_kv` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o
```

- [ ] **Step 4: Touch `nvs_kv` symbols in `esp-gate/main/main.c`** — add the include after the existing `#include "mqtt_topic.h"` line:

```c
#include "nvs_kv.h"
```

and add this block at the end of `app_main` (just before its closing `}`):

```c
    /* nvs_kv compile-gate: touch enough of the surface to link it. */
    (void) nvs_kv_init();
    nvs_kv kv;
    if (nvs_kv_open(&kv, "node", true) == ESP_OK) {
        char ssid[33];
        (void) nvs_kv_get_str(&kv, "ssid", ssid, sizeof ssid, "default-ap");
        (void) nvs_kv_set_u32(&kv, "boot", 1);
        (void) nvs_kv_commit(&kv);
        nvs_kv_close(&kv);
    }
```

- [ ] **Step 5: Run the L1 host gate (must stay green)**

Run:
```sh
cd esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: four `... 0 Failures ... OK` lines and `strict: OK`.

- [ ] **Step 6: Run the esp-gate compile-gate (the GREEN for this task)**

Run (set the build env from Global Constraints first if not already set):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig >/dev/null && make -j4 2>&1 | tail -20
```
Expected: ends with the link step producing `build/esp_libs_gate.elf` (a line like `LD build/esp_libs_gate.elf` then esptool `Generating ...bin`). Confirm the ELF exists:
```sh
ls -l build/esp_libs_gate.elf
```
Expected: the file exists (non-zero size). If the link fails on an unresolved nvs/esp-mqtt include, add `COMPONENT_REQUIRES := nvs_flash` to `esp-libs/component.mk` and rebuild.

- [ ] **Step 7: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/nvs_kv/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/nvs_kv): reusable NVS key/value wrapper (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: `mqtt_node` wrapper (LWT presence) + wire into component & gate

**Files:**
- Create: `esp-libs/mqtt_node/mqtt_node.h`
- Create: `esp-libs/mqtt_node/mqtt_node.c`
- Modify: `esp-libs/component.mk` (add `mqtt_node` to all three lists)
- Modify: `esp-gate/main/main.c` (touch `mqtt_node` symbols)
- Test: none (compile-gate only). GREEN = `esp-gate` links.

**Interfaces:**
- Consumes: SDK `mqtt_client.h` (esp-mqtt: `esp_mqtt_client_handle_t`, `esp_mqtt_client_config_t`, `esp_mqtt_client_init/start/stop/publish/subscribe/register_event`, `esp_mqtt_event_handle_t`, `MQTT_EVENT_*`, `MQTT_EVENT_ANY`), `esp_event.h` (`esp_event_base_t`, `esp_event_handler_t` signature), `esp_err.h`, `esp_log.h`. Independent of Task 1.
- Produces:
  ```c
  typedef enum { MQTT_NODE_DISCONNECTED, MQTT_NODE_CONNECTED } mqtt_node_state;
  typedef void (*mqtt_node_state_cb)(mqtt_node_state state, void *ctx);
  typedef void (*mqtt_node_msg_cb)(const char *topic, int topic_len,
                                   const char *data, int data_len, void *ctx);
  typedef struct { /* see header */ } mqtt_node_config;
  esp_err_t       mqtt_node_start(const mqtt_node_config *cfg);
  esp_err_t       mqtt_node_stop(void);
  int             mqtt_node_publish(const char *topic, const char *data, int len, int qos, int retain);
  int             mqtt_node_subscribe(const char *topic, int qos);
  mqtt_node_state mqtt_node_get_state(void);
  ```

- [ ] **Step 1: Write `esp-libs/mqtt_node/mqtt_node.h`**

```c
#ifndef CLIBS_MQTT_NODE_H
#define CLIBS_MQTT_NODE_H

#include "esp_err.h"

/* Singleton MQTT manager for a smart-home node: connect to the gateway broker,
 * publish/subscribe, and (optionally) announce presence via a retained "online"
 * message plus a Last-Will "offline". Relies on esp-mqtt's built-in
 * auto-reconnect (no custom backoff). Mirrors wifi_sta's shape. */
typedef enum { MQTT_NODE_DISCONNECTED, MQTT_NODE_CONNECTED } mqtt_node_state;

typedef void (*mqtt_node_state_cb)(mqtt_node_state state, void *ctx);

/* Inbound message. topic/data are NOT NUL-terminated — use the given lengths. */
typedef void (*mqtt_node_msg_cb)(const char *topic, int topic_len,
                                 const char *data, int data_len, void *ctx);

/* All string pointers must stay valid for the lifetime of the connection
 * (the wrapper keeps the config by shallow copy — use literals or static storage). */
typedef struct {
    const char *uri;             /* e.g. "mqtt://192.168.4.1:1883" (required) */
    const char *client_id;       /* NULL -> esp-mqtt default */
    const char *username;        /* NULL -> no auth */
    const char *password;        /* NULL -> no auth */
    int         keepalive;       /* 0 -> SDK default (120 s) */
    /* presence (LWT) — leave presence_topic NULL to disable this whole block */
    const char *presence_topic;  /* e.g. "home/<room>/<node>/online" */
    const char *online_msg;      /* RETAINED publish on connect (e.g. "1") */
    const char *offline_msg;     /* Last-Will message (e.g. "0") */
    int         presence_qos;
    mqtt_node_state_cb state_cb; /* optional (may be NULL) */
    mqtt_node_msg_cb   msg_cb;   /* optional (may be NULL) */
    void              *cb_ctx;
} mqtt_node_config;

/* Init (once) + start the client. NULL cfg/uri -> ESP_ERR_INVALID_ARG. Idempotent. */
esp_err_t mqtt_node_start(const mqtt_node_config *cfg);

/* Stop the client (keeps it initialised for a later restart). */
esp_err_t mqtt_node_stop(void);

/* Publish. len 0 -> esp-mqtt computes strlen(data). Returns msg_id (>=0) or -1. */
int mqtt_node_publish(const char *topic, const char *data, int len, int qos, int retain);

/* Subscribe. Returns msg_id (>=0) or -1. */
int mqtt_node_subscribe(const char *topic, int qos);

mqtt_node_state mqtt_node_get_state(void);

#endif /* CLIBS_MQTT_NODE_H */
```

- [ ] **Step 2: Write `esp-libs/mqtt_node/mqtt_node.c`**

```c
#include "mqtt_node.h"

#include "mqtt_client.h"   /* SDK esp-mqtt (no name clash: our header is mqtt_node.h) */
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "mqtt_node";
static bool                     s_inited;
static esp_mqtt_client_handle_t s_client;
static mqtt_node_state          s_state = MQTT_NODE_DISCONNECTED;
static mqtt_node_config         s_cfg;   /* shallow copy; caller keeps strings alive */

static void event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void) arg;
    (void) base;
    esp_mqtt_event_handle_t e = (esp_mqtt_event_handle_t) event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
    case MQTT_EVENT_CONNECTED:
        s_state = MQTT_NODE_CONNECTED;
        if (s_cfg.state_cb) {
            s_cfg.state_cb(MQTT_NODE_CONNECTED, s_cfg.cb_ctx);
        }
        if (s_cfg.presence_topic != NULL && s_cfg.online_msg != NULL) {
            esp_mqtt_client_publish(s_client, s_cfg.presence_topic, s_cfg.online_msg,
                                    0, s_cfg.presence_qos, 1 /* retain */);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_state = MQTT_NODE_DISCONNECTED;
        if (s_cfg.state_cb) {
            s_cfg.state_cb(MQTT_NODE_DISCONNECTED, s_cfg.cb_ctx);
        }
        break;
    case MQTT_EVENT_DATA:
        if (s_cfg.msg_cb) {
            s_cfg.msg_cb(e->topic, e->topic_len, e->data, e->data_len, s_cfg.cb_ctx);
        }
        break;
    default:
        break;
    }
}

esp_err_t mqtt_node_start(const mqtt_node_config *cfg)
{
    if (cfg == NULL || cfg->uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_cfg = *cfg;   /* shallow copy of the config (incl. string pointers) */

    if (!s_inited) {
        esp_mqtt_client_config_t mc = {0};
        mc.uri = cfg->uri;
        if (cfg->client_id != NULL) mc.client_id = cfg->client_id;
        if (cfg->username  != NULL) mc.username  = cfg->username;
        if (cfg->password  != NULL) mc.password  = cfg->password;
        if (cfg->keepalive != 0)    mc.keepalive = cfg->keepalive;
        if (cfg->presence_topic != NULL) {
            mc.lwt_topic  = cfg->presence_topic;
            mc.lwt_msg    = cfg->offline_msg;
            mc.lwt_qos    = cfg->presence_qos;
            mc.lwt_retain = 1;
        }
        s_client = esp_mqtt_client_init(&mc);
        if (s_client == NULL) {
            ESP_LOGE(TAG, "esp_mqtt_client_init failed");
            return ESP_FAIL;
        }
        esp_err_t err = esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY,
                                                       event_handler, NULL);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "register_event failed: 0x%x", err);
            return err;
        }
        s_inited = true;
    }
    return esp_mqtt_client_start(s_client);
}

esp_err_t mqtt_node_stop(void)
{
    if (!s_inited || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_state = MQTT_NODE_DISCONNECTED;
    return esp_mqtt_client_stop(s_client);
}

int mqtt_node_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    if (!s_inited || s_client == NULL || topic == NULL) {
        return -1;
    }
    return esp_mqtt_client_publish(s_client, topic, data, len, qos, retain);
}

int mqtt_node_subscribe(const char *topic, int qos)
{
    if (!s_inited || s_client == NULL || topic == NULL) {
        return -1;
    }
    return esp_mqtt_client_subscribe(s_client, topic, qos);
}

mqtt_node_state mqtt_node_get_state(void)
{
    return s_state;
}
```

- [ ] **Step 3: Wire `mqtt_node` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o
```

- [ ] **Step 4: Touch `mqtt_node` symbols in `esp-gate/main/main.c`** — add the include after `#include "nvs_kv.h"`:

```c
#include "mqtt_node.h"
```

and add this block at the end of `app_main` (just before its closing `}`):

```c
    /* mqtt_node compile-gate: touch enough of the surface to link it. */
    mqtt_node_config mcfg = {
        .uri = "mqtt://192.168.4.1:1883",
        .client_id = "node-1",
        .presence_topic = "home/livingroom/node-1/online",
        .online_msg = "1", .offline_msg = "0", .presence_qos = 1,
        .state_cb = 0, .msg_cb = 0, .cb_ctx = 0,
    };
    (void) mqtt_node_start(&mcfg);
    (void) mqtt_node_subscribe("home/livingroom/node-1/set", 1);
    (void) mqtt_node_publish("home/livingroom/node-1/state", "on", 0, 1, 1);
    ESP_LOGI(TAG, "mqtt state=%d", (int) mqtt_node_get_state());
```

- [ ] **Step 5: Run the esp-gate compile-gate (the GREEN for this task)**

Run (build env from Global Constraints set):
```sh
cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -20 && ls -l build/esp_libs_gate.elf
```
Expected: the link step produces `build/esp_libs_gate.elf` and the `ls` shows a non-zero file. If the build fails on unresolved esp-mqtt symbols/includes, add `mqtt` (and keep `nvs_flash`) to `COMPONENT_REQUIRES` in `esp-libs/component.mk`:
```makefile
COMPONENT_REQUIRES := nvs_flash mqtt
```
then rebuild.

- [ ] **Step 6: Confirm the new symbols are in the ELF**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-gate
xtensa-lx106-elf-nm build/esp_libs_gate.elf | grep -E " T (mqtt_node_start|mqtt_node_publish|nvs_kv_init)"
```
Expected: all three appear as `T` (text) symbols.

- [ ] **Step 7: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/mqtt_node/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/mqtt_node): MQTT manager + LWT presence on esp-mqtt (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: Final gates + README update

**Files:**
- Modify: `esp-libs/README.md` (document `nvs_kv` + `mqtt_node` under Layer 2)
- Test: re-run both gates (L1 host + esp-gate link).

**Interfaces:**
- Consumes: nothing new. Documents the public APIs produced by Tasks 1 & 2.
- Produces: nothing (docs only).

- [ ] **Step 1: Re-run the L1 host gate**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: four `... 0 Failures ... OK` lines + `strict: OK`.

- [ ] **Step 2: Re-run the esp-gate compile-gate**

Run (env set):
```sh
cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -5 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`, file exists.

- [ ] **Step 3: Update `esp-libs/README.md`** — make these three edits:

(a) Replace the Layer-2 bullet in the intro list:
```
- *Layer 2 — SDK wrappers* (wifi / mqtt-client / nvs) — later, needs the SDK.
```
with:
```
- **Layer 2 — SDK wrappers (this set, compile-gated):** `wifi_sta`, `mqtt_node`,
  `nvs_kv` — thin ESP8266-RTOS-SDK glue, verified by the `esp-gate` link (not
  host-testable, no hardware).
```

(b) Immediately after the existing `### wifi_sta — first L2 wrapper` subsection (before `### Compile-gate`), insert:
```markdown
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
```

(c) In the `### Compile-gate` subsection, change the line:
```
cd esp-gate && make -j4   # produces build/esp_libs_gate.elf
```
to:
```
cd esp-gate && make defconfig && make -j4   # produces build/esp_libs_gate.elf
```

- [ ] **Step 4: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document Layer-2 nvs_kv + mqtt_node

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:**
- `mqtt_node` wrapper (singleton, state/msg callbacks, LWT presence, built-in reconnect) → Task 2. ✓
- `nvs_kv` reusable key/value (init/open/get-with-default/set/commit/close) → Task 1. ✓
- `component.mk` adds both dirs to all 3 lists, `test_*.c` excluded → Tasks 1 & 2 Step 3. ✓
- `esp-gate/main.c` symbol-touch for both → Tasks 1 & 2 Step 4. ✓
- Compile-gate only, L1 host tests stay green → every task's gate steps. ✓
- README Layer-2 note → Task 3. ✓
- Out of scope (node_core, typed config, TLS) → not present. ✓

**Placeholder scan:** No TBD/TODO; all code blocks complete; commands have expected output. ✓

**Type consistency:** `nvs_kv` struct + 8 functions consistent across header/impl/gate-touch (Task 1). `mqtt_node` enum/types/6 functions consistent across header/impl/gate-touch (Task 2). `component.mk` Task 2 state is the superset of Task 1 (adds `mqtt_node` after `nvs_kv`). `main.c` Task 2 includes `nvs_kv.h` from Task 1 (sequential). ✓

**Deviation from spec naming:** the spec calls the wrapper `mqtt_client`; this plan renames it `mqtt_node` to avoid shadowing the SDK header `mqtt_client.h` (see Global Constraints). Flag this to the user before execution.
