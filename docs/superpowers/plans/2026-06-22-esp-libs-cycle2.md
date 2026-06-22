# esp-libs Cycle 2 (Layer-2 infra + wifi_sta) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `esp-libs/` a buildable ESP8266 component with a `make` compile-gate project (`esp-gate/`), and add the `wifi_sta` Layer-2 wrapper (STA connect + reconnect spaced by `backoff`), verified by cross-compile/link for the ESP8266 target.

**Architecture:** esp-libs builds two ways from the same sources — host-Unity for L1 logic (unchanged), and as the `esp_libs` ESP8266 component (`xtensa-lx106`) built by `esp-gate`. L2 wrappers `#include` SDK headers, so they are **compile-gate-only** (no host test, no hardware). De-risk the new build infra first (a `wifi_sta` stub that links), then drop in the real SDK glue.

**Tech Stack:** ESP8266 RTOS SDK v3.4, `xtensa-lx106-elf-gcc` 8.4.0 (Rosetta), legacy `make` build, FreeRTOS, esp_wifi/esp_event/tcpip_adapter.

**Spec:** `clibs/docs/superpowers/specs/2026-06-22-esp-libs-cycle2-wifi-sta-design.md`

## Global Constraints
- Work in `/Users/man.nk/git/clibs`. `esp-libs/component.mk` + `esp-gate/` are committed DIRECTLY in clibs (not a submodule). Branch created by the controller; implementers must NOT switch branches.
- **The compile-gate is run via the legacy `make` build with this env (export in every gate command — shell state does not persist):**
  ```sh
  export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
  export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
  source ~/esp/idf-venv/bin/activate
  ```
  Gate = `cd esp-gate && make defconfig && make` produces `build/esp_libs_gate.elf` (links → PASS). Do NOT use `idf.py` (broken on this arm64 host).
- L1 host tests (`esp-libs/Makefile` → `make test`/`strict`) must stay green and are NOT modified.
- `wifi_sta` is compile-gate-only: success = it cross-compiles + links in `esp-gate`. No host test of the SDK glue; no hardware run.
- `CLIBS_*_H` guards; wrapper uses `esp_log` for diagnostics. Append to every commit message:
  ```
  Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
  ```

## File Structure

| File | Responsibility |
|------|----------------|
| `esp-libs/component.mk` | declares esp-libs as ESP8266 component `esp_libs` (lib `.o`s only) |
| `esp-libs/wifi_sta/wifi_sta.{h,c}` | STA wrapper (stub in T1, real in T2) |
| `esp-gate/Makefile` | compile-gate project (`PROJECT_NAME` + `EXTRA_COMPONENT_DIRS`) |
| `esp-gate/main/main.c`, `main/component.mk` | tiny app referencing the libs so they link |
| `esp-gate/components/esp_libs` | symlink → `../../esp-libs` |
| `esp-gate/.gitignore` | `build/`, `sdkconfig`, `sdkconfig.old` |

---

## Task 1: Component infra + `esp-gate` + `wifi_sta` stub (establish the gate links)

**Files:**
- Create: `esp-libs/component.mk`, `esp-libs/wifi_sta/wifi_sta.h`, `esp-libs/wifi_sta/wifi_sta.c`, `esp-gate/Makefile`, `esp-gate/.gitignore`, `esp-gate/main/main.c`, `esp-gate/main/component.mk`, `esp-gate/components/esp_libs` (symlink)

**Interfaces:**
- Produces: `wifi_sta_state` enum + `wifi_sta_config` struct + `esp_err_t wifi_sta_start(const wifi_sta_config*)`, `esp_err_t wifi_sta_stop(void)`, `wifi_sta_state wifi_sta_get_state(void)` (stubbed in this task).

- [ ] **Step 1: Create `esp-libs/wifi_sta/wifi_sta.h`** (real API; used by the stub and the real impl):
```c
#ifndef CLIBS_WIFI_STA_H
#define CLIBS_WIFI_STA_H

#include <stdint.h>
#include "esp_err.h"

/* STA-mode WiFi manager: connect to an AP and reconnect with exponential backoff. */
typedef enum { WIFI_STA_DISCONNECTED, WIFI_STA_CONNECTING, WIFI_STA_CONNECTED } wifi_sta_state;
typedef void (*wifi_sta_cb)(wifi_sta_state state, void *ctx);

typedef struct {
    const char  *ssid;
    const char  *password;        /* may be NULL for open AP */
    uint32_t     backoff_base_ms; /* reconnect backoff base, e.g. 500 */
    uint32_t     backoff_max_ms;  /* reconnect backoff cap,  e.g. 30000 */
    wifi_sta_cb  cb;              /* optional status callback (may be NULL) */
    void        *cb_ctx;
} wifi_sta_config;

esp_err_t      wifi_sta_start(const wifi_sta_config *cfg);
esp_err_t      wifi_sta_stop(void);
wifi_sta_state wifi_sta_get_state(void);

#endif /* CLIBS_WIFI_STA_H */
```

- [ ] **Step 2: Create `esp-libs/wifi_sta/wifi_sta.c`** — STUB (compiles with no SDK calls; real glue lands in Task 2):
```c
#include "wifi_sta.h"

/* STUB (cycle-2 Task 1): establishes the build/link gate. Real STA logic in Task 2. */
static wifi_sta_state s_state = WIFI_STA_DISCONNECTED;

esp_err_t wifi_sta_start(const wifi_sta_config *cfg) {
    if (cfg == NULL || cfg->ssid == NULL) return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

esp_err_t wifi_sta_stop(void) {
    s_state = WIFI_STA_DISCONNECTED;
    return ESP_OK;
}

wifi_sta_state wifi_sta_get_state(void) {
    return s_state;
}
```

- [ ] **Step 3: Create `esp-libs/component.mk`**
```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o
```

- [ ] **Step 4: Create the `esp-gate` project files**

`esp-gate/Makefile`:
```makefile
PROJECT_NAME := esp_libs_gate
EXTRA_COMPONENT_DIRS := $(abspath $(CURDIR)/components)
include $(IDF_PATH)/make/project.mk
```

`esp-gate/.gitignore`:
```
build/
sdkconfig
sdkconfig.old
```

`esp-gate/main/component.mk` (empty — default behaviour):
```makefile
#
# "main" component — default build of all sources in this dir.
#
```

`esp-gate/main/main.c` (references each lib so the linker pulls the whole `esp_libs` component):
```c
#include "esp_log.h"
#include "wifi_sta.h"
#include "backoff.h"
#include "debounce.h"
#include "filter.h"
#include "mqtt_topic.h"

static const char *TAG = "esp_libs_gate";

void app_main(void)
{
    /* This project exists only to cross-compile + link esp-libs for the ESP8266
       target (a compile-gate). It is never expected to run on hardware. We touch
       a symbol from each lib so the linker pulls them in. */

    backoff b;
    backoff_init(&b, 500, 30000);
    ESP_LOGI(TAG, "backoff first=%u", (unsigned) backoff_next(&b));

    debounce d;
    debounce_init(&d, 3, 0);
    (void) debounce_update(&d, 1);

    ema f;
    ema_init(&f, 3, 0);
    (void) ema_update(&f, 100);

    char topic[32];
    (void) mqtt_topic_join(topic, sizeof topic, (const char *const[]){"home", "node", "state"}, 3);
    (void) mqtt_topic_match("home/#", topic);

    wifi_sta_config cfg = {
        .ssid = "gateway-ap", .password = "secret",
        .backoff_base_ms = 500, .backoff_max_ms = 30000,
        .cb = 0, .cb_ctx = 0,
    };
    (void) wifi_sta_start(&cfg);
    ESP_LOGI(TAG, "state=%d", (int) wifi_sta_get_state());
}
```

- [ ] **Step 5: Create the component symlink**
```bash
cd /Users/man.nk/git/clibs/esp-gate
mkdir -p components
ln -s ../../esp-libs components/esp_libs
ls -l components/esp_libs   # should show: esp_libs -> ../../esp-libs
```

- [ ] **Step 6: Run the compile-gate (the GREEN for this task)**
```bash
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd /Users/man.nk/git/clibs/esp-gate
make defconfig && make -j4 2>&1 | tail -25
ls -la build/esp_libs_gate.elf && echo GATE_OK
```
Expected: build completes, `LD build/esp_libs_gate.elf`, `GATE_OK`. If the build complains about an unresolved `esp_libs` component, missing include, or the symlink, fix `component.mk`/`Makefile`/symlink until it links (this is the point of the task — proving the infra). If it needs `COMPONENT_REQUIRES`, add it to `component.mk`.

- [ ] **Step 7: Confirm L1 host tests are unaffected**
```bash
cd /Users/man.nk/git/clibs/esp-libs && make clean && make test 2>&1 | grep -E "Tests|OK" | tail -8 && make strict 2>&1 | tail -1
```
Expected: backoff/debounce/filter/mqtt_topic suites all `0 Failures`; `strict: OK`.

- [ ] **Step 8: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/component.mk esp-libs/wifi_sta esp-gate
git commit -m "build(esp-libs): ESP8266 component + esp-gate compile-gate + wifi_sta stub"
```

---

## Task 2: `wifi_sta` real implementation (STA connect + reconnect-backoff)

Replace the stub body with the real SDK glue. The GREEN remains the `esp-gate` link. Mirror `$IDF_PATH/examples/wifi/getting_started/station/main/station_example_main.c` for exact SDK symbol names/signatures, and adjust whatever the compiler/linker requires (e.g. the precise header for `IP_EVENT`, or `ESP_IF_WIFI_STA` vs `WIFI_IF_STA`).

**Files:**
- Modify: `esp-libs/wifi_sta/wifi_sta.c`

**Interfaces:**
- Consumes: `backoff` (`backoff_init/next/reset` from `esp-libs/backoff/backoff.h`, same component).

- [ ] **Step 1: Replace `esp-libs/wifi_sta/wifi_sta.c` with the real implementation**
```c
#include "wifi_sta.h"
#include "backoff.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

static const char *TAG = "wifi_sta";

static wifi_sta_cb            s_cb;
static void                 *s_cb_ctx;
static backoff               s_backoff;
static volatile wifi_sta_state s_state = WIFI_STA_DISCONNECTED;
static TaskHandle_t          s_reconnect_task;

static void set_state(wifi_sta_state st) {
    s_state = st;
    if (s_cb) s_cb(st, s_cb_ctx);
}

/* Runs off the event-loop thread so it can sleep for the backoff delay. */
static void reconnect_task(void *arg) {
    (void) arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     /* wait for a disconnect signal */
        uint32_t delay_ms = backoff_next(&s_backoff);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        set_state(WIFI_STA_CONNECTING);
        esp_wifi_connect();
    }
}

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void) arg; (void) data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        set_state(WIFI_STA_CONNECTING);
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        set_state(WIFI_STA_DISCONNECTED);
        if (s_reconnect_task) xTaskNotifyGive(s_reconnect_task);  /* schedule a spaced retry */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        backoff_reset(&s_backoff);
        set_state(WIFI_STA_CONNECTED);
        ESP_LOGI(TAG, "got IP, connected");
    }
}

esp_err_t wifi_sta_start(const wifi_sta_config *cfg) {
    if (cfg == NULL || cfg->ssid == NULL) return ESP_ERR_INVALID_ARG;

    s_cb = cfg->cb;
    s_cb_ctx = cfg->cb_ctx;
    backoff_init(&s_backoff, cfg->backoff_base_ms, cfg->backoff_max_ms);
    s_state = WIFI_STA_DISCONNECTED;

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wc = {0};
    strncpy((char *) wc.sta.ssid, cfg->ssid, sizeof(wc.sta.ssid));
    if (cfg->password)
        strncpy((char *) wc.sta.password, cfg->password, sizeof(wc.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wc));

    if (s_reconnect_task == NULL)
        xTaskCreate(reconnect_task, "wifi_sta_recon", 2048, NULL, 5, &s_reconnect_task);

    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

esp_err_t wifi_sta_stop(void) {
    esp_err_t err = esp_wifi_stop();
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
    if (s_reconnect_task) {
        vTaskDelete(s_reconnect_task);
        s_reconnect_task = NULL;
    }
    s_state = WIFI_STA_DISCONNECTED;
    return err;
}

wifi_sta_state wifi_sta_get_state(void) {
    return s_state;
}
```

- [ ] **Step 2: Run the compile-gate (GREEN)**
```bash
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd /Users/man.nk/git/clibs/esp-gate
make -j4 2>&1 | tail -25
ls -la build/esp_libs_gate.elf && echo GATE_OK
```
Expected: links, `GATE_OK`. If a symbol/header mismatch appears (e.g. `IP_EVENT` needs a different include, or `ESP_IF_WIFI_STA` is named `WIFI_IF_STA` in this SDK), consult the station example + the SDK headers under `$IDF_PATH/components` and fix the source until it links. Do NOT stub anything out to force a link — the wrapper must be the real implementation.

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/wifi_sta/wifi_sta.c
git commit -m "feat(esp-libs/wifi_sta): STA connect + backoff-spaced reconnect (compile-gated)"
```

---

## Task 3: Final gates + README

**Files:** Modify `esp-libs/README.md`.

- [ ] **Step 1: Run both gates**
```bash
# host L1 tests
cd /Users/man.nk/git/clibs/esp-libs && make clean && make test 2>&1 | grep -E "Tests|OK" | tail -8 && make strict 2>&1 | tail -1
# ESP compile-gate
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate
cd /Users/man.nk/git/clibs/esp-gate && make -j4 >/dev/null 2>&1 && ls build/esp_libs_gate.elf && echo GATE_OK
```
Expected: 4 L1 suites `0 Failures`, `strict: OK`, and `GATE_OK`. If anything fails, STOP and report BLOCKED.

- [ ] **Step 2: Update `esp-libs/README.md`** — READ it, then add a short **Layer 2** section: esp-libs is now also an ESP8266 component (`component.mk`); `wifi_sta` is the first L2 wrapper (STA + backoff reconnect); the compile-gate is the `esp-gate/` project — `cd esp-gate && make` (with the SDK env). Note L2 wrappers are compile-gate-only (SDK glue; no host test until hardware). Keep the existing L1 content. Match the markdown style.

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): note Layer-2 component + wifi_sta + esp-gate compile-gate"
```

- [ ] **Step 4: STOP — controller handles integration.** Do NOT merge/push. Report DONE so the controller runs the final review, merges to clibs main, and pushes.

---

## Self-Review notes
- **Spec coverage:** component.mk + esp-gate (symlinked `esp_libs`) → T1; wifi_sta stub→real (STA + backoff reconnect task) → T1/T2; compile-gate (`make` link) is the GREEN; L1 host tests preserved → T1 step 7 + T3; README → T3. No host test / hardware for SDK glue (per spec).
- **Interfaces:** `wifi_sta_start/stop/get_state` + `wifi_sta_config`/`wifi_sta_state` identical in header, stub, real impl, and gate `main.c`. `wifi_sta` consumes `backoff_init/next/reset`.
- **Placeholder scan:** none — full code provided; the only "adjust as needed" is the deliberate compile-gate iteration on exact SDK symbol names (the gate build is the check), which is not a placeholder but the verification method for untestable SDK glue.
- **Build-env note repeated** in every gate step (shell state doesn't persist across the subagent's commands).
- **Out of scope (untouched):** mqtt/nvs_config wrappers, provisioning, hardware/runtime; common/, avr-libs, esp-idf-template.
