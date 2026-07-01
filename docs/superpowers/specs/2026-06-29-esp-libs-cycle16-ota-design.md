# esp-libs cycle 16 — OTA control-plane (ota_version + ota_manifest + ota_fsm) + ota_dev glue Design

**Status:** approved (brainstorm 2026-06-29)

**Goal:** Add over-the-air firmware update to esp-libs for ESP8266 smart-home nodes as three host-Unity-tested **pure control-plane** libs (`ota_version`, `ota_manifest`, `ota_fsm`) plus one SDK glue lib (`ota_dev`, esp_ota_ops + HTTP fetch + reboot/rollback — COMPILE_GATE deferred). Lean control-plane scope (option A).

## Background

The roadmap's "big cycle". Node topology (per [[project-esp8266-smarthome]]): WiFi STA + MQTT client to a LAN gateway/broker, **no internet, no cloud TLS**. So OTA is LAN-local: the gateway publishes an MQTT command carrying a JSON manifest; the node fetches the firmware over plain HTTP from the gateway and flashes it via the ESP8266 RTOS SDK's `esp_ota_ops` (ota_0/ota_1 two-app partition scheme; the bootloader picks the boot partition).

OTA's host-testable surface is thinner than a device driver: the **decision + sequencing logic** is pure (version compare, manifest validation, a download/verify/apply state machine); the **effects** (flash write, HTTP, sha256, reboot, rollback) are SDK glue. This cycle delivers the pure control-plane host-tested + the glue written-but-deferred, matching every prior cycle's "pure tested + glue deferred" pattern. sha256 verification lives in the glue (mbedtls on-target); a pure sha256 was considered and deferred (option B).

**Naming convention (cycle 13+):** pure `<lib>.{h,c}` (freestanding, named in `test_<lib>.c`, no SDK) + glue `<lib>_dev.{h,c}` (SDK). Only `ota_dev` here.

## Architecture

Data flow (one node):
1. Gateway → MQTT publish `home/<room>/<node>/ota/set`, payload `{"version":"1.3.0","url":"http://192.168.4.1/fw/node-1.3.0.bin","size":421000,"sha256":"<64 hex>"}`.
2. App's mqtt_node msg_cb routes that topic to `ota_handle_manifest(json,len)`.
3. Glue extracts fields (json_get/cJSON) → `ota_manifest_valid(...)` → `ota_version_is_newer(current, candidate)`. If not valid or not newer → `ota_fsm` REJECT → IDLE (ignore).
4. If accepted → `ota_fsm` ACCEPT → DOWNLOADING; glue `esp_ota_begin` + `esp_http_client` GET, streaming each chunk to `esp_ota_write` and `ota_fsm` DATA(n) for progress.
5. On complete → VERIFYING; glue finalizes sha256 over the written image, compares to the manifest → VERIFIED (or FAIL).
6. VERIFIED → APPLYING; glue `esp_ota_end` + `esp_ota_set_boot_partition` → APPLIED → DONE → `esp_restart`.
7. Next boot: app self-checks and calls `ota_mark_valid` (rollback-cancel); a crash-loop before mark-valid lets the bootloader roll back to the previous partition.

Three pure libs under embedded-sprint: `ota_version` and `ota_fsm` are independent (`deps: []`); `ota_manifest` calls `ota_version_parse`, so it depends on `ota_version` (`deps: [ota_version]`) — the workflow runs {version, fsm} in the first layer, then manifest. `ota_dev` is the glue (deferred). Integration wires Makefile (test 38 → **41**), component.mk, esp-gate, README.

### Conventions (per profile-clibs)
- Pure files freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`, `<string.h>` allowed for strlen/strncmp), no SDK, no malloc, host-testable.
- Glue includes `esp_err.h`, `esp_ota_ops.h`, `esp_http_client.h`, `esp_system.h`, plus `mqtt_node.h`/`json_get.h`.
- `CLIBS_<MODULE>_H` guards; NULL-guard every pointer param; doc comment per public fn; "Not reentrant".
- **No UB:** version parse bounded; progress integer math guards divide-by-zero; no left-shift of a possibly-negative value.

## C16-1 `ota_version` (pure)

`esp-libs/ota_version/ota_version.{h,c}`:
```c
typedef struct { uint16_t major, minor, patch; } ota_version;

/* Parse "MAJOR.MINOR.PATCH" (exactly three decimal components, each <= 65535).
 * Trailing/invalid chars or wrong component count -> false, *out untouched. NULL -> false. */
bool ota_version_parse(const char *s, ota_version *out);

int  ota_version_cmp(ota_version a, ota_version b);              /* major,minor,patch lexicographic -> -1/0/1 */
bool ota_version_is_newer(ota_version current, ota_version candidate);   /* cmp(candidate,current) > 0 */
```
Vectors: `parse("1.2.3")`→{1,2,3} true; `parse("10.0.255")`→{10,0,255}; `parse("1.2")`→false; `parse("1.2.3.4")`→false; `parse("1.2.x")`→false; `parse("")`/NULL→false; `cmp({1,2,3},{1,2,4})`=−1; `cmp({2,0,0},{1,9,9})`=1; `cmp({1,2,3},{1,2,3})`=0; `is_newer({1,2,3},{1,3,0})`=true; `is_newer({1,2,3},{1,2,3})`=false; `is_newer({1,2,3},{1,2,2})`=false.

## C16-2 `ota_manifest` (pure validator)

`esp-libs/ota_manifest/ota_manifest.{h,c}`:
```c
/* Validate already-extracted manifest fields (the glue does the cJSON extraction).
 * true iff: version parses (ota_version_parse); url != NULL, begins "http://", len > 7;
 * 0 < size <= max_size; sha256_hex != NULL, strlen == 64, every char in [0-9a-f].
 * Any failure -> false. */
bool ota_manifest_valid(const char *version, const char *url, uint32_t size,
                        const char *sha256_hex, uint32_t max_size);
```
Consumes `ota_version_parse` (host suite links `ota_manifest.c` + `ota_version.c`). Vectors (max_size 1048576, a valid 64-hex string H): `valid("1.3.0","http://192.168.4.1/fw.bin",421000,H,max)`→true; `size 0`→false; `size 1048577`→false; `sha256` 63 chars→false; `sha256` with `'G'`/uppercase→false; `url "https://x"`→false; `url "ftp://x"`→false; `url "http://"` (len 7)→false; `version "1.2"`→false; NULL version/url/sha256→false.

## C16-3 `ota_fsm` (pure state machine + progress)

`esp-libs/ota_fsm/ota_fsm.{h,c}`:
```c
typedef enum { OTA_IDLE, OTA_CHECKING, OTA_DOWNLOADING, OTA_VERIFYING, OTA_APPLYING, OTA_DONE, OTA_FAILED } ota_state;
typedef enum { OTA_EV_START, OTA_EV_ACCEPT, OTA_EV_REJECT, OTA_EV_DATA, OTA_EV_DOWNLOADED, OTA_EV_VERIFIED, OTA_EV_APPLIED, OTA_EV_FAIL } ota_event;
typedef struct { ota_state state; uint32_t total; uint32_t received; } ota_fsm;

void      ota_fsm_init(ota_fsm *self);                            /* OTA_IDLE, total=received=0; NULL -> no-op */
ota_state ota_fsm_state(const ota_fsm *self);                     /* OTA_FAILED if NULL */
ota_state ota_fsm_on(ota_fsm *self, ota_event ev, uint32_t arg);  /* apply transition; returns new state; NULL -> OTA_FAILED */
uint8_t   ota_fsm_progress(const ota_fsm *self);                  /* received*100/total; 0 if total==0; capped 100; NULL -> 0 */
```
Transition table (arg: `total` on START, `nbytes` on DATA; otherwise ignored):
| state | event | next | effect |
|---|---|---|---|
| IDLE | START | CHECKING | total=arg, received=0 |
| CHECKING | ACCEPT | DOWNLOADING | |
| CHECKING | REJECT | IDLE | |
| DOWNLOADING | DATA | DOWNLOADING | received += arg |
| DOWNLOADING | DOWNLOADED | VERIFYING | |
| VERIFYING | VERIFIED | APPLYING | |
| APPLYING | APPLIED | DONE | |
| IDLE/CHECKING/DOWNLOADING/VERIFYING/APPLYING | FAIL | FAILED | |
| DONE / FAILED | any | (unchanged) | terminal, absorb |
| any | unexpected event | (unchanged) | defensive no-op |

`ota_fsm_progress` = `total ? (uint8_t)(received >= total ? 100 : (uint32_t)((uint64_t)received * 100u / total)) : 0` (int64 widen so `received*100` can't overflow uint32).
Vectors: init→IDLE, progress 0; `on(START,1000)`→CHECKING (total 1000, received 0, progress 0); `on(ACCEPT)`→DOWNLOADING; `on(DATA,250)`×2→received 500, progress 50; `on(DOWNLOADED)`→VERIFYING; `on(VERIFIED)`→APPLYING; `on(APPLIED)`→DONE; from CHECKING `on(REJECT)`→IDLE; from DOWNLOADING `on(FAIL)`→FAILED; DONE `on(START)`→DONE (absorbed); progress total0→0; received>total→100; NULL self → state OTA_FAILED / progress 0.

## `ota_dev` glue (deferred)

`esp-libs/ota_dev/ota_dev.{h,c}`:
```c
typedef struct { ota_version current; ota_fsm fsm; uint32_t max_size; } ota;
esp_err_t ota_init(ota *self, ota_version current, uint32_t max_size);
/* Handle an MQTT OTA manifest payload: json_get extract {version,url,size,sha256} ->
 * ota_manifest_valid + ota_version_is_newer -> drive ota_fsm; if accepted, esp_ota_begin +
 * esp_http_client GET url, stream chunks to esp_ota_write (+ fsm DATA), sha256(mbedtls) vs
 * manifest, esp_ota_set_boot_partition, esp_restart. */
esp_err_t ota_handle_manifest(ota *self, const char *json, size_t len);
esp_err_t ota_mark_valid(ota *self);   /* rollback-cancel on a healthy boot */
```
Not host-compiled; COMPILE_GATE deferred. esp-gate touches `ota_init`/`ota_handle_manifest`/`ota_mark_valid`.

## Testing & gates

- 3 new pure Unity suites (`test_ota_version`, `test_ota_manifest`, `test_ota_fsm`) → `make -C esp-libs test` **38 → 41** (`ota_manifest` suite links `ota_manifest.c` + `ota_version.c`).
- `make strict` (-Werror) on every pure `.c`; `make sanitize` (UBSan+ASan, `ASAN_OPTIONS=detect_leaks=0` on darwin) clean — the progress int math is the main UB-risk surface.
- `ota_dev` glue: COMPILE_GATE **deferred** (xtensa absent), substituted by an adversarial reviewer pass (esp_ota_ops sequence, http fetch loop, fsm wiring, json extraction, NULL-safety).

## Out of scope (deferred)

- Pure sha256 lib (option B) — verification stays in the glue via mbedtls.
- Delta/compressed OTA, signature (RSA/ECDSA) verification, multi-node broadcast/staged rollout.
- The actual rollback semantics on ESP8266 RTOS SDK v3.4 (bootloader valid-flag) — verified on hardware.
- On-hardware run / real esp_ota flash + HTTP + reboot for the whole flow.

## Execution

1. `writing-plans` → implementation plan (3 pure libs + glue + integration; no Task-0 prereq).
2. `embedded-sprint` → 3 parallel chains → Integration (test→41) → Sprint Review → Retro → merge (user-gated) → push → memory. Runs on the patched engine (Integrator produces README + pathspec commits).
