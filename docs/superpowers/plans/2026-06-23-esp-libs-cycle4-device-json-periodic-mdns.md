# esp-libs Cycle 4 — `device_id` + `json` + `periodic` + `mdns_node` Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four Layer-2 wrappers to the `esp_libs` component — `device_id` (MAC→stable id), `json` (no-malloc builder + cJSON getters), `periodic` (esp_timer wrapper), `mdns_node` (mDNS init/resolve) — with the two pure parts host-Unity-tested and all SDK glue verified by the `esp-gate` link.

**Architecture:** Each wrapper is one directory under `esp-libs/`. The pure logic (`device_id_format`, the `json_build` builder) lives in SDK-header-free `.c` files that the host `Makefile` compiles + tests with Unity; the SDK glue (`device_id_get`, `json_get`, `periodic`, `mdns_node`) lives in separate `.c` files compiled only by the `esp_libs` component and exercised by symbol-touches in `esp-gate/main/main.c`. Per-task GREEN: host `make test` passes AND `esp-gate` links `build/esp_libs_gate.elf`.

**Tech Stack:** ESP8266_RTOS_SDK v3.4 (xtensa-lx106-elf gcc via Rosetta), legacy `make`; SDK components `esp8266` (`esp_read_mac`, `esp_timer`), `json` (cJSON), `mdns`, `lwip` (`ip4addr_ntoa`); host `cc` + vendored Unity.

## Global Constraints

- **Build env (run once per shell before any ESP gate build, as ONE command — the shell does NOT persist `export`s between separate Bash calls):**
  ```sh
  export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4
  ```
  The first ESP build takes a few minutes — let it finish. Run `make defconfig` once if `sdkconfig` is missing.
- **Two test postures:** pure logic (`device_id_format`, `json_build`) is **host-Unity-tested** (real RED→GREEN). SDK glue (`device_id_get`, `json_get`, `periodic`, `mdns_node`) is **compile-gate only** — do NOT write host tests for it and do NOT fabricate empty tests; the `esp-gate` link is its verification.
- **Pure `.c` files must NOT include any SDK header** (`esp_err.h`, `esp_system.h`, `cJSON.h`, …) so the host `cc` can compile them. SDK return types/decls go in a *separate* SDK-only header (e.g. `device_id_get.h`, `json_get.h`).
- **Naming:** `mdns_node` (NOT `mdns`) — the SDK header is `mdns.h`; a same-named wrapper would shadow it. Same rule that produced `mqtt_node`.
- **clibs conventions:** header guards `CLIBS_<NAME>_H`; `esp_err_t` returns for SDK glue, `int` length/sentinel for pure formatters; NULL-arg guards; caller-provided transparent struct storage, no malloc (`json_build`, `periodic`); diagnostics via `esp_log` in SDK glue (never `common/log`).
- **`component.mk` excludes `test_*.c`** (explicit `COMPONENT_OBJS`). If the ESP build fails on unresolved includes/symbols, add the needed component(s) to `COMPONENT_REQUIRES` (candidates: `nvs_flash mqtt mdns json esp_timer`) — the gate is the check.
- **Host gate:** `cd esp-libs && make test` must end this cycle with **6 suites** (backoff, debounce, filter, mqtt_topic, device_id, json_build), all `0 Failures`; `make strict` → `strict: OK`. Do not touch L1 sources, `wifi_sta`, `mqtt_node`, `nvs_kv`, `common/`, `avr-libs/`, `esp-idf-template/`.
- **Commits:** end each message with the trailer `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

---

### Task 1: `device_id` — MAC→stable id (pure format host-tested + SDK get)

**Files:**
- Create: `esp-libs/device_id/device_id.h` (pure decl), `esp-libs/device_id/device_id.c` (pure), `esp-libs/device_id/device_id_get.h` (SDK decl), `esp-libs/device_id/device_id_get.c` (SDK), `esp-libs/device_id/test_device_id.c` (host test)
- Modify: `esp-libs/Makefile` (add `device_id` host target), `esp-libs/component.mk`, `esp-gate/main/main.c`

**Interfaces:**
- Consumes: SDK `esp_system.h` (`esp_read_mac`, `ESP_MAC_WIFI_STA`), `esp_err.h`.
- Produces:
  ```c
  int       device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix); /* device_id.h */
  esp_err_t device_id_get(char *buf, size_t n, const char *prefix);                          /* device_id_get.h */
  ```

- [ ] **Step 1: Write the failing host test `esp-libs/device_id/test_device_id.c`**

```c
#include "unity.h"
#include "device_id.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_format_with_prefix(void)
{
    uint8_t mac[6] = {0, 1, 2, 3, 4, 5};
    char buf[16];
    int len = device_id_format(mac, buf, sizeof buf, "esp-");
    TEST_ASSERT_EQUAL_INT(10, len);
    TEST_ASSERT_EQUAL_STRING("esp-030405", buf);   /* last 3 bytes: 03 04 05 */
}

static void test_format_no_prefix(void)
{
    uint8_t mac[6] = {0xaa, 0xbb, 0xcc, 0x3c, 0x71, 0xbf};
    char buf[16];
    int len = device_id_format(mac, buf, sizeof buf, NULL);
    TEST_ASSERT_EQUAL_INT(6, len);
    TEST_ASSERT_EQUAL_STRING("3c71bf", buf);
}

static void test_format_truncation(void)
{
    uint8_t mac[6] = {0, 1, 2, 3, 4, 5};
    char buf[4];
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, buf, sizeof buf, "esp-"));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

static void test_format_null(void)
{
    char buf[16];
    uint8_t mac[6] = {0};
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(NULL, buf, sizeof buf, "esp-"));
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, NULL, 16, "esp-"));
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, buf, 0, "esp-"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_format_with_prefix);
    RUN_TEST(test_format_no_prefix);
    RUN_TEST(test_format_truncation);
    RUN_TEST(test_format_null);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/device_id/device_id.h`**

```c
#ifndef CLIBS_DEVICE_ID_H
#define CLIBS_DEVICE_ID_H

#include <stdint.h>
#include <stddef.h>

/* Format the LAST 3 bytes of `mac` as lowercase hex after `prefix`, into buf[n]
 * (NUL-terminated). e.g. prefix "esp-", mac {..,0x3c,0x71,0xbf} -> "esp-3c71bf".
 * prefix NULL -> no prefix. Returns the string length (excl NUL), or -1 on
 * truncation / NULL mac|buf / n==0. Pure: no SDK headers (host-testable). */
int device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix);

#endif /* CLIBS_DEVICE_ID_H */
```

- [ ] **Step 3: Run the test to verify it FAILS (no implementation yet)**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Idevice_id -o build/test_device_id device_id/test_device_id.c device_id/device_id.c ../common/third_party/unity/unity.c
```
Expected: FAIL — link/compile error `device_id.c: No such file` (or undefined `device_id_format`).

- [ ] **Step 4: Write `esp-libs/device_id/device_id.c` (pure implementation)**

```c
#include "device_id.h"

#include <string.h>

static char hex_digit(uint8_t nib)
{
    return (nib < 10) ? (char) ('0' + nib) : (char) ('a' + nib - 10);
}

int device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix)
{
    if (mac == NULL || buf == NULL || n == 0) {
        return -1;
    }
    size_t plen = (prefix != NULL) ? strlen(prefix) : 0;
    if (plen + 6 + 1 > n) {       /* prefix + 6 hex chars + NUL */
        buf[0] = '\0';
        return -1;
    }
    size_t i = 0;
    for (size_t p = 0; p < plen; p++) {
        buf[i++] = prefix[p];
    }
    for (size_t b = 3; b < 6; b++) {            /* last 3 MAC bytes */
        buf[i++] = hex_digit((uint8_t) (mac[b] >> 4));
        buf[i++] = hex_digit((uint8_t) (mac[b] & 0x0F));
    }
    buf[i] = '\0';
    return (int) i;
}
```

- [ ] **Step 5: Run the test to verify it PASSES**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make device_id
```
Expected: `4 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Write the SDK-glue header `esp-libs/device_id/device_id_get.h`**

```c
#ifndef CLIBS_DEVICE_ID_GET_H
#define CLIBS_DEVICE_ID_GET_H

#include "esp_err.h"
#include "device_id.h"

/* Read the station MAC (esp_read_mac, ESP_MAC_WIFI_STA) and device_id_format it
 * into buf[n] with `prefix`. NULL buf / n==0 -> ESP_ERR_INVALID_ARG; a too-small
 * buffer -> ESP_ERR_INVALID_SIZE (buf set to ""). */
esp_err_t device_id_get(char *buf, size_t n, const char *prefix);

#endif /* CLIBS_DEVICE_ID_GET_H */
```

- [ ] **Step 7: Write the SDK-glue impl `esp-libs/device_id/device_id_get.c`**

```c
#include "device_id_get.h"

#include "esp_system.h"   /* esp_read_mac, ESP_MAC_WIFI_STA */

esp_err_t device_id_get(char *buf, size_t n, const char *prefix)
{
    if (buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        buf[0] = '\0';
        return err;
    }
    if (device_id_format(mac, buf, n, prefix) < 0) {
        buf[0] = '\0';
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}
```

- [ ] **Step 8: Wire the `device_id` host target into `esp-libs/Makefile`** — make three edits:

(a) Add `device_id` to the `.PHONY` line and the `test:` aggregate:
```
.PHONY: test backoff debounce filter mqtt_topic device_id strict clean
test: backoff debounce filter mqtt_topic device_id
```
(b) Add this target after the `mqtt_topic:` target block:
```makefile
device_id: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idevice_id -o $(BUILD)/test_device_id \
	    device_id/test_device_id.c device_id/device_id.c $(UNITY)/unity.c
	./$(BUILD)/test_device_id
```
(c) Add this line to the `strict:` recipe (before the `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Idevice_id -c device_id/device_id.c -o /dev/null
```

- [ ] **Step 9: Wire `device_id` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node device_id
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node device_id
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o
```

- [ ] **Step 10: Touch `device_id` in `esp-gate/main/main.c`** — add after `#include "mqtt_node.h"`:

```c
#include "device_id_get.h"
```
and add at the end of `app_main` (just before the closing `}`):

```c
    /* device_id compile-gate. */
    char id[24];
    (void) device_id_get(id, sizeof id, "esp-");
    ESP_LOGI(TAG, "id=%s", id);
```

- [ ] **Step 11: Run the L1/host gate + the ESP compile-gate**

Run host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: FIVE `... 0 Failures ... OK` lines (incl. device_id) + `strict: OK`.

Run ESP gate (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make defconfig >/dev/null && make -j4 2>&1 | tail -8 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf` (file exists). If unresolved, add `COMPONENT_REQUIRES := nvs_flash mqtt` (esp_system is implicit) and rebuild.

- [ ] **Step 12: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/device_id/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/device_id): MAC->stable id (host-tested format + SDK get)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: `json` — no-malloc builder (host-tested) + cJSON getters

**Files:**
- Create: `esp-libs/json/json_build.h`, `esp-libs/json/json_build.c`, `esp-libs/json/test_json_build.c`, `esp-libs/json/json_get.h`, `esp-libs/json/json_get.c`
- Modify: `esp-libs/Makefile` (add `json_build` host target), `esp-libs/component.mk`, `esp-gate/main/main.c`

**Interfaces:**
- Consumes: SDK `cJSON.h` (`cJSON_Parse`, `cJSON_GetObjectItem`, `cJSON_IsString`, `cJSON_IsNumber`, `valuestring`, `valueint`, `cJSON_Delete`).
- Produces:
  ```c
  /* json_build.h */
  typedef struct { char *buf; size_t cap; size_t len; bool err; bool first; } json_build;
  void json_build_init(json_build *b, char *buf, size_t cap);
  void json_build_str(json_build *b, const char *key, const char *val);
  void json_build_int(json_build *b, const char *key, int32_t val);
  void json_build_raw(json_build *b, const char *key, const char *raw);
  int  json_build_end(json_build *b);
  /* json_get.h */
  int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def);
  int json_get_int(const char *json, const char *key, int def);
  ```

- [ ] **Step 1: Write the failing host test `esp-libs/json/test_json_build.c`**

```c
#include "unity.h"
#include "json_build.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_basic_object(void)
{
    char buf[64];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "state", "on");
    json_build_int(&b, "rssi", -60);
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"state\":\"on\",\"rssi\":-60}", buf);
    TEST_ASSERT_EQUAL_INT((int) strlen(buf), len);
}

static void test_raw_member(void)
{
    char buf[32];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_raw(&b, "ok", "true");
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"ok\":true}", buf);
    TEST_ASSERT_TRUE(len > 0);
}

static void test_empty_object(void)
{
    char buf[8];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{}", buf);
    TEST_ASSERT_EQUAL_INT(2, len);
}

static void test_escaping(void)
{
    char buf[32];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "k", "a\"b\\c");
    (void) json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"k\":\"a\\\"b\\\\c\"}", buf);
}

static void test_overflow(void)
{
    char buf[8];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "state", "on");   /* will not fit in 8 bytes */
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_INT(-1, len);
    TEST_ASSERT_TRUE(b.err);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_object);
    RUN_TEST(test_raw_member);
    RUN_TEST(test_empty_object);
    RUN_TEST(test_escaping);
    RUN_TEST(test_overflow);
    return UNITY_END();
}
```

- [ ] **Step 2: Write `esp-libs/json/json_build.h`**

```c
#ifndef CLIBS_JSON_BUILD_H
#define CLIBS_JSON_BUILD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* No-malloc JSON object builder into a caller buffer:
 *   json_build b; json_build_init(&b, buf, sizeof buf);
 *   json_build_str(&b, "state", "on"); json_build_int(&b, "rssi", -60);
 *   int len = json_build_end(&b);   // len >= 0, or -1 on overflow
 * Pure C, no SDK headers (host-testable). */
typedef struct {
    char  *buf;
    size_t cap;     /* buffer capacity */
    size_t len;     /* current length (excl NUL) */
    bool   err;     /* sticky overflow/arg error */
    bool   first;   /* no comma before the first member */
} json_build;

void json_build_init(json_build *b, char *buf, size_t cap);          /* writes "{" */
void json_build_str(json_build *b, const char *key, const char *val);/* "key":"val" (escapes " and \) */
void json_build_int(json_build *b, const char *key, int32_t val);    /* "key":123 */
void json_build_raw(json_build *b, const char *key, const char *raw);/* "key":raw (caller-formed) */
int  json_build_end(json_build *b);   /* writes "}"; returns len (excl NUL), or -1 if err/overflow */

#endif /* CLIBS_JSON_BUILD_H */
```

- [ ] **Step 3: Run the test to verify it FAILS**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -g -I../common/third_party/unity -Ijson -o build/test_json_build json/test_json_build.c json/json_build.c ../common/third_party/unity/unity.c
```
Expected: FAIL — `json_build.c: No such file` (or undefined `json_build_*`).

- [ ] **Step 4: Write `esp-libs/json/json_build.c`**

```c
#include "json_build.h"

#include <stdio.h>
#include <string.h>

/* Append n bytes; set sticky err on overflow (needs room for the trailing NUL). */
static void append(json_build *b, const char *s, size_t n)
{
    if (b->err) {
        return;
    }
    if (b->len + n >= b->cap) {       /* need buf[len+n] for NUL too */
        b->err = true;
        return;
    }
    memcpy(b->buf + b->len, s, n);
    b->len += n;
    b->buf[b->len] = '\0';
}

/* Append a quoted JSON string value, escaping " and \. */
static void append_escaped(json_build *b, const char *s)
{
    append(b, "\"", 1);
    for (const char *p = s; *p != '\0' && !b->err; p++) {
        if (*p == '"' || *p == '\\') {
            char esc[2] = { '\\', *p };
            append(b, esc, 2);
        } else {
            append(b, p, 1);
        }
    }
    append(b, "\"", 1);
}

static void begin_member(json_build *b, const char *key)
{
    if (!b->first) {
        append(b, ",", 1);
    }
    b->first = false;
    append_escaped(b, key);
    append(b, ":", 1);
}

void json_build_init(json_build *b, char *buf, size_t cap)
{
    if (b == NULL) {
        return;
    }
    b->buf = buf;
    b->cap = cap;
    b->len = 0;
    b->first = true;
    b->err = (buf == NULL || cap == 0);
    if (!b->err) {
        b->buf[0] = '\0';
        append(b, "{", 1);
    }
}

void json_build_str(json_build *b, const char *key, const char *val)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL || val == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    append_escaped(b, val);
}

void json_build_int(json_build *b, const char *key, int32_t val)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    char num[12];   /* fits -2147483648 + NUL */
    int n = snprintf(num, sizeof num, "%ld", (long) val);
    if (n < 0) {
        b->err = true;
        return;
    }
    append(b, num, (size_t) n);
}

void json_build_raw(json_build *b, const char *key, const char *raw)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL || raw == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    append(b, raw, strlen(raw));
}

int json_build_end(json_build *b)
{
    if (b == NULL) {
        return -1;
    }
    append(b, "}", 1);
    return b->err ? -1 : (int) b->len;
}
```

- [ ] **Step 5: Run the test to verify it PASSES**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make json_build
```
Expected: `5 Tests 0 Failures 0 Ignored` / `OK`.

- [ ] **Step 6: Write the cJSON getter header `esp-libs/json/json_get.h`**

```c
#ifndef CLIBS_JSON_GET_H
#define CLIBS_JSON_GET_H

#include <stddef.h>

/* Parse `json`, read string field `key` into buf[n] (NUL-term, truncated to fit).
 * Missing / not-a-string / parse-fail -> copy `def` (NULL->""), return its length.
 * On success return the copied length. -1 on NULL json|key|buf or n==0. */
int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def);

/* Parse `json`, read integer field `key`. Missing / not-a-number / parse-fail -> def. */
int json_get_int(const char *json, const char *key, int def);

#endif /* CLIBS_JSON_GET_H */
```

- [ ] **Step 7: Write the cJSON getter impl `esp-libs/json/json_get.c`**

```c
#include "json_get.h"

#include <string.h>

#include "cJSON.h"

static int copy_into(char *buf, size_t n, const char *src)
{
    size_t len = strlen(src);
    if (len >= n) {
        len = n - 1;            /* truncate to fit */
    }
    memcpy(buf, src, len);
    buf[len] = '\0';
    return (int) len;
}

int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def)
{
    if (json == NULL || key == NULL || buf == NULL || n == 0) {
        return -1;
    }
    const char *result = (def != NULL) ? def : "";
    cJSON *root = cJSON_Parse(json);
    if (root != NULL) {
        cJSON *item = cJSON_GetObjectItem(root, key);
        if (item != NULL && cJSON_IsString(item) && item->valuestring != NULL) {
            result = item->valuestring;
        }
        int len = copy_into(buf, n, result);
        cJSON_Delete(root);
        return len;
    }
    return copy_into(buf, n, result);
}

int json_get_int(const char *json, const char *key, int def)
{
    if (json == NULL || key == NULL) {
        return def;
    }
    int result = def;
    cJSON *root = cJSON_Parse(json);
    if (root != NULL) {
        cJSON *item = cJSON_GetObjectItem(root, key);
        if (item != NULL && cJSON_IsNumber(item)) {
            result = item->valueint;
        }
        cJSON_Delete(root);
    }
    return result;
}
```

- [ ] **Step 8: Wire the `json_build` host target into `esp-libs/Makefile`** — three edits:

(a) Add `json_build` to `.PHONY` and `test:`:
```
.PHONY: test backoff debounce filter mqtt_topic device_id json_build strict clean
test: backoff debounce filter mqtt_topic device_id json_build
```
(b) Add this target after the `device_id:` target block:
```makefile
json_build: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ijson -o $(BUILD)/test_json_build \
	    json/test_json_build.c json/json_build.c $(UNITY)/unity.c
	./$(BUILD)/test_json_build
```
(c) Add to the `strict:` recipe (before `@echo`):
```makefile
	$(CC) -std=c11 -Wall -Wextra -Werror -Ijson -c json/json_build.c -o /dev/null
```

- [ ] **Step 9: Wire `json` into `esp-libs/component.mk`** — replace the whole file with:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node device_id json
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node device_id json
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o
```

- [ ] **Step 10: Touch `json` in `esp-gate/main/main.c`** — add after `#include "device_id_get.h"`:

```c
#include "json_build.h"
#include "json_get.h"
```
and add at the end of `app_main` (before the closing `}`):

```c
    /* json compile-gate. */
    char payload[64];
    json_build jb;
    json_build_init(&jb, payload, sizeof payload);
    json_build_str(&jb, "state", "on");
    json_build_int(&jb, "rssi", -60);
    (void) json_build_end(&jb);
    char cmd[16];
    (void) json_get_str("{\"cmd\":\"on\"}", "cmd", cmd, sizeof cmd, "off");
    (void) json_get_int("{\"level\":42}", "level", 0);
    ESP_LOGI(TAG, "payload=%s cmd=%s", payload, cmd);
```

- [ ] **Step 11: Run both gates**

Host:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: SIX `... 0 Failures ... OK` lines + `strict: OK`.

ESP gate (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -8 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`. If unresolved cJSON symbols, add `json` to `COMPONENT_REQUIRES` in `esp-libs/component.mk` and rebuild.

- [ ] **Step 12: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/json/ esp-libs/Makefile esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs/json): no-malloc builder (host-tested) + cJSON getters

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: `periodic` + `mdns_node` (both compile-gate)

**Files:**
- Create: `esp-libs/periodic/periodic.h`, `esp-libs/periodic/periodic.c`, `esp-libs/mdns_node/mdns_node.h`, `esp-libs/mdns_node/mdns_node.c`
- Modify: `esp-libs/component.mk`, `esp-gate/main/main.c`
- Test: none (SDK glue, compile-gate only). GREEN = `esp-gate` links + host tests still 6 suites green.

**Interfaces:**
- Consumes: SDK `esp_timer.h` (`esp_timer_handle_t`, `esp_timer_cb_t`, `esp_timer_create_args_t`, `esp_timer_create/start_periodic/stop/delete`); `mdns.h` (`mdns_init`, `mdns_hostname_set`, `mdns_query_a`); `lwip/ip4_addr.h` (`ip4_addr_t`, `ip4addr_ntoa`); `esp_err.h`.
- Produces:
  ```c
  /* periodic.h */
  typedef struct { esp_timer_handle_t h; bool started; } periodic;
  typedef void (*periodic_cb)(void *ctx);
  esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx);
  esp_err_t periodic_stop(periodic *self);
  /* mdns_node.h */
  esp_err_t mdns_node_init(const char *hostname);
  esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms);
  ```

- [ ] **Step 1: Write `esp-libs/periodic/periodic.h`**

```c
#ifndef CLIBS_PERIODIC_H
#define CLIBS_PERIODIC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_timer.h"   /* for esp_timer_handle_t */

/* Periodic-callback helper over esp_timer. Caller owns the `periodic` storage. */
typedef struct {
    esp_timer_handle_t h;
    bool               started;
} periodic;

/* Runs in the esp_timer task — keep it short and non-blocking. */
typedef void (*periodic_cb)(void *ctx);

/* Create + start a timer firing every period_ms. NULL self|cb -> ESP_ERR_INVALID_ARG;
 * already started -> ESP_ERR_INVALID_STATE. */
esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx);

/* Stop + delete the timer. NULL self -> ESP_ERR_INVALID_ARG; not started -> ESP_OK (no-op). */
esp_err_t periodic_stop(periodic *self);

#endif /* CLIBS_PERIODIC_H */
```

- [ ] **Step 2: Write `esp-libs/periodic/periodic.c`**

```c
#include "periodic.h"

esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx)
{
    if (self == NULL || cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_timer_create_args_t args = {
        .callback = cb,         /* periodic_cb has the same signature as esp_timer_cb_t */
        .arg      = ctx,
        .name     = "periodic",
    };
    esp_err_t err = esp_timer_create(&args, &self->h);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(self->h, (uint64_t) period_ms * 1000);
    if (err != ESP_OK) {
        esp_timer_delete(self->h);
        return err;
    }
    self->started = true;
    return ESP_OK;
}

esp_err_t periodic_stop(periodic *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_OK;
    }
    esp_timer_stop(self->h);
    esp_timer_delete(self->h);
    self->started = false;
    return ESP_OK;
}
```

- [ ] **Step 3: Write `esp-libs/mdns_node/mdns_node.h`**

```c
#ifndef CLIBS_MDNS_NODE_H
#define CLIBS_MDNS_NODE_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Initialise mDNS and (optionally) set the hostname. hostname NULL -> just init.
 * (Named mdns_node to avoid shadowing the SDK header mdns.h.) */
esp_err_t mdns_node_init(const char *hostname);

/* Resolve `host` (A record) to a dotted-quad string in ip_buf[n]. timeout in ms.
 * NULL host|ip_buf or n==0 -> ESP_ERR_INVALID_ARG; on lookup failure returns the
 * SDK error with ip_buf set to ""; result too long -> ESP_ERR_INVALID_SIZE. */
esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms);

#endif /* CLIBS_MDNS_NODE_H */
```

- [ ] **Step 4: Write `esp-libs/mdns_node/mdns_node.c`**

```c
#include "mdns_node.h"

#include <string.h>

#include "mdns.h"
#include "lwip/ip4_addr.h"   /* ip4_addr_t, ip4addr_ntoa */

esp_err_t mdns_node_init(const char *hostname)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        return err;
    }
    if (hostname != NULL) {
        err = mdns_hostname_set(hostname);
    }
    return err;
}

esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms)
{
    if (host == NULL || ip_buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ip_buf[0] = '\0';
    ip4_addr_t addr;
    memset(&addr, 0, sizeof addr);
    esp_err_t err = mdns_query_a(host, timeout_ms, &addr);
    if (err != ESP_OK) {
        return err;
    }
    const char *s = ip4addr_ntoa(&addr);   /* static buffer — copy immediately */
    size_t len = strlen(s);
    if (len >= n) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(ip_buf, s, len + 1);
    return ESP_OK;
}
```

- [ ] **Step 5: Wire both into `esp-libs/component.mk`** — replace the whole file with the final version:

```makefile
# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
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

- [ ] **Step 6: Touch both in `esp-gate/main/main.c`** — three edits:

(a) Add after `#include "json_get.h"`:
```c
#include "periodic.h"
#include "mdns_node.h"
```
(b) Add this static callback at file scope, immediately after the `static const char *TAG = ...;` line:
```c
static void gate_tick(void *ctx) { (void) ctx; }
```
(c) Add at the end of `app_main` (before the closing `}`):
```c
    /* periodic compile-gate. */
    periodic pt;
    (void) periodic_start(&pt, 5000, gate_tick, 0);
    (void) periodic_stop(&pt);

    /* mdns_node compile-gate. */
    (void) mdns_node_init("node-1");
    char bip[16];
    (void) mdns_node_resolve("broker.local", bip, sizeof bip, 1000);
    ESP_LOGI(TAG, "broker ip=%s", bip);
```

- [ ] **Step 7: Run both gates**

Host (must still be 6 suites — periodic/mdns add no host tests):
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -cE "0 Failures" && make strict 2>&1 | tail -1
```
Expected: `6` and `strict: OK`.

ESP gate (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -8 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`. If unresolved `mdns_*`/`esp_timer_*`/`ip4addr_ntoa`, set `COMPONENT_REQUIRES := nvs_flash mqtt mdns json` in `esp-libs/component.mk` (esp_timer + lwip are implicit) and rebuild.

- [ ] **Step 8: Confirm the new symbols are in the ELF**

Run (env's PATH in the same command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; cd /Users/man.nk/git/clibs/esp-gate && xtensa-lx106-elf-nm build/esp_libs_gate.elf | grep -E " T (device_id_get|json_get_str|periodic_start|mdns_node_init)"
```
Expected: all four appear as `T` symbols.

- [ ] **Step 9: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/periodic/ esp-libs/mdns_node/ esp-libs/component.mk esp-gate/main/main.c
git commit -m "feat(esp-libs): periodic (esp_timer) + mdns_node wrappers (compile-gated)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: Final gates + README update

**Files:**
- Modify: `esp-libs/README.md`
- Test: re-run both gates.

**Interfaces:** Consumes nothing new; documents the public APIs from Tasks 1–3. Produces nothing.

- [ ] **Step 1: Re-run the host gate**

Run:
```sh
cd /Users/man.nk/git/clibs/esp-libs && make clean >/dev/null && make test 2>&1 | grep -E "Failures|OK" && make strict 2>&1 | tail -1
```
Expected: SIX `... 0 Failures ... OK` lines + `strict: OK`.

- [ ] **Step 2: Re-run the ESP compile-gate**

Run (env as ONE command):
```sh
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"; export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"; source ~/esp/idf-venv/bin/activate; cd /Users/man.nk/git/clibs/esp-gate && make -j4 2>&1 | tail -5 && ls -l build/esp_libs_gate.elf
```
Expected: links `build/esp_libs_gate.elf`, file exists.

- [ ] **Step 3: Update `esp-libs/README.md`** — two edits:

(a) Replace the Layer-2 intro bullet (currently):
```
- **Layer 2 — SDK wrappers (this set, compile-gated):** `wifi_sta`, `mqtt_node`,
  `nvs_kv` — thin ESP8266-RTOS-SDK glue, verified by the `esp-gate` link (not
  host-testable, no hardware).
```
with:
```
- **Layer 2 — SDK wrappers (this set):** `wifi_sta`, `mqtt_node`, `nvs_kv`,
  `device_id`, `json`, `periodic`, `mdns_node` — ESP8266-RTOS-SDK glue verified by
  the `esp-gate` link; the pure parts (`device_id_format`, the `json` builder) are
  host-Unity-tested.
```

(b) Insert the following just before the `### Compile-gate` subsection (after the `### mqtt_node …` subsection):
```markdown
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
```

- [ ] **Step 4: Commit**

```sh
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): document Layer-2 device_id + json + periodic + mdns_node

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage:**
- `device_id` pure format (host-tested) + SDK get → Task 1. ✓
- `json` no-malloc builder (host-tested) + cJSON getters → Task 2. ✓
- `periodic` esp_timer wrapper → Task 3. ✓
- `mdns_node` init + resolve → Task 3. ✓
- `component.mk` adds 4 dirs to all 3 lists, `test_*.c` excluded → Tasks 1/2/3 wiring. ✓
- `Makefile` +2 host targets, `make test` = 6 suites, strict covers both pure `.c` → Tasks 1 & 2. ✓
- `esp-gate/main.c` touches all 4 → Tasks 1/2/3. ✓
- README documents the 4 → Task 4. ✓
- Pure `.c` SDK-header-free (host-compilable); SDK decls in separate headers → Task 1 splits `device_id.h`/`device_id_get.h`, Task 2 splits `json_build.h`/`json_get.h`. ✓
- Out of scope (OTA, node_core) → absent. ✓

**Placeholder scan:** No TBD/TODO; every code/step is complete; commands have expected output. ✓

**Type consistency:** `device_id_format`/`device_id_get` signatures match across header/impl/test/gate. `json_build` struct + 5 funcs and `json_get` 2 funcs consistent across header/impl/test/gate. `periodic` struct + 2 funcs and `mdns_node` 2 funcs consistent across header/impl/gate. `component.mk` is an incremental superset per task (Task1 +device_id, Task2 +json, Task3 +periodic+mdns_node = the spec's full list). `main.c` includes accumulate (device_id_get.h → json_*.h → periodic.h/mdns_node.h) and the `gate_tick` static is added in Task 3 before its first use. ✓

**Boundary check:** `json_build` `append` rejects when `len+n >= cap` (reserves the NUL slot) — verified: a value ending exactly at `cap-1` index is accepted, `cap` is rejected. `device_id_format` needs `plen+6+1 <= n`. `copy_into`/`mdns` bound with `len >= n` → `ESP_ERR_INVALID_SIZE`/truncate. ✓
