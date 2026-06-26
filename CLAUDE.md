# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

`clibs` is a **workspace** of reusable C libraries (libraries only — node apps build in their own repos) for building IoT/embedded firmware on **ATmega (AVR)** and **ESP (ESP8266 / ESP-IDF)** microcontrollers. It is a super-repo of several parts built with *different* toolchains — there is no single top-level build:

| Path | Ownership | Platform / toolchain | Produces |
|------|-----------|----------------------|----------|
| `common/` | clibs's own code (committed here) | portable C99, host GCC | host test executables |
| `esp-libs/` | clibs's own code (committed here) | **ESP8266** RTOS SDK (xtensa-lx106) + host GCC for L1 | ESP8266 SDK component; host Unity test executables |
| `esp-gate/` | clibs's own code (committed here) | ESP8266 RTOS SDK (xtensa-lx106), legacy `make` | `esp_libs_gate.elf` (link-only gate, not flashed) |
| `avr-libs/` | clibs's own code (committed here) | ATmega328p @ 16 MHz, `avr-gcc`/`avr-libc` via Eclipse AVR plugin; host GCC for the extracted pure logic | static libs (`lib*.a`); host Unity test executables |
| `esp-idf-template/` | **git submodule** (`mannk98/esp-idf-template`, branch `master`) | ESP32 family, ESP-IDF (`idf.py`/CMake or legacy make) | ESP firmware |

**Active focus:** `esp-libs/` — ESP8266 smart-home node building blocks, built bottom-up over cycles (see `docs/superpowers/` plans+specs and git history). The `common/` + `esp-libs/` host tests are what CI runs (`.github/workflows/ci.yml`, gcc/ubuntu); the AVR and xtensa toolchains are out of CI scope.

`esp-idf-template/` is the only remaining submodule (**pinned to a commit**, no branch tracking). Clone with submodules:

```sh
git clone --recurse-submodules git@mannk98:mannk98/clibs.git
git submodule update --init --recursive   # if already cloned without --recurse-submodules
```

## Working across the submodule boundary

`common/`, `esp-libs/`, `esp-gate/` and `avr-libs/` are committed directly here. Only `esp-idf-template/` is an independent repo mounted as a submodule (its gitdir lives under `.git/modules/esp-idf-template`). To change it:

1. Commit **and push inside the submodule first** — it has its own remote.
2. Then in `clibs`: `git add esp-idf-template && git commit` to record the new pinned commit.

After editing the submodule, `git status` in `clibs` shows it as "new commits", not as file diffs. (`avr-libs/` used to be a submodule too; it was absorbed as plain files — its full history still lives in its old remote `mannk98/avr-libs`.)

## Build & test — `common/` (portable C, host)

Each library is a self-contained folder named `<lib>.c/` (`dlinklist.c`, `log.c`, `ringbuffchar.c`) holding `src/` (the real `.c`+`.h` API), a `main.c` assert-based test harness, and — except `log` — an Eclipse-CDT `Debug/` makefile. **"Running the tests" = compiling one lib's `main.c` against its `src/` and executing it** (success prints `function <name> pass`; there is no test framework).

```sh
# log — builds clean on any modern compiler
gcc -std=c99 -Wall -o /tmp/log_demo common/log.c/example/main.c common/log.c/src/log.c
/tmp/log_demo somefile.txt          # a `cat` that also logs to stderr + log.txt
```

⚠️ `dlinklist` and `ringbuffchar` were written for an older GCC and **do not compile cleanly on Apple clang / modern GCC** unchanged. The committed `Debug/<lib>.c` files are stale **Linux x86-64 ELF** build artifacts (not source — ignore/rebuild them).

```sh
# ringbuffchar: src/str_utils.c calls strlen() without <string.h> → force-include it.
# Builds AND the test passes here (exit 0) — but it allocates ~10M nodes and sleep(10)
# twice, so the run takes ~20s and a lot of RAM.
gcc -Os -Wall -include string.h -o /tmp/rb_test \
    common/ringbuffchar.c/main.c common/ringbuffchar.c/src/ringbuffchar.c \
    common/ringbuffchar.c/src/str_utils.c && /tmp/rb_test

# dlinklist: only COMPILES with the warning demoted — main.c passes int(char*,char*)
# where find() wants int(void*,void*), which is genuine UB. The bundled test then
# SEGFAULTS at runtime on modern macOS/clang, so the smoke test is broken here
# (fix main.c's comparator signature before trusting it). Build (do not expect a pass):
gcc -std=c99 -Wall -Wno-error=incompatible-function-pointer-types \
    -o /tmp/dll_test common/dlinklist.c/main.c common/dlinklist.c/src/dlinklist.c
```

The Eclipse path (`cd common/<lib>.c/Debug && make all`) only succeeds on the original Linux GCC host; on a modern toolchain pass `make CFLAGS='-O0 -g3 -Wall -Wno-error=incompatible-function-pointer-types' all`. `make clean` works everywhere.

To **reuse** a library elsewhere, take only its `src/*.c`+`*.h` (`ringbuffchar` also needs `src/str_utils.c`); `main.c` and `Debug/` are scaffolding. Design conventions shared across all three libs: caller-controlled memory via a `freefunc` passed to `*_init` (`NULL` for literals/stack, `free` for `malloc`'d payloads); typed error enums (`llErrTypes`/`rb_err_t`); a cooperative `isBusy` flag that is **not** real thread-safety.

## Build & test — `esp-libs/` + `esp-gate/` (ESP8266)

`esp-libs/` is the active component: reusable ESP8266 (RTOS SDK, FreeRTOS) blocks in three layers — **L1** pure logic (portable C, host-Unity-tested), **L2** SDK wrappers (`wifi_sta`, `mqtt_node`, `nvs_kv`, `device_id`, `json`, `periodic`, `mdns_node`, `adc_a0`, `i2c_bus`, `spi_bus`), **L3** device drivers (`relay`, `button`, `dht`, `pwm_dimmer`, `ds18b20`, `servo`, `hcsr04`). Each lib is its own folder; the pure part of an L2/L3 lib lives in a separate `*_<noun>.c/.h` (e.g. `dht_parse.c`, `servo_duty.c`) so it can be host-tested apart from the SDK glue. Convention: no-malloc, caller-owned transparent struct + `self`-methods, NULL-guarded, `CLIBS_<MODULE>_H` include guards. **Not yet run on real hardware** — bit-bang/1-Wire timing is a first cut to tune on a chip.

```sh
make -C esp-libs test      # build + run all host Unity suites (18 currently); each prints "OK"
make -C esp-libs strict    # -Werror compile gate on every pure .c
make -C esp-libs clean
```

Only the pure (L1 + the `*_<noun>` files of L2/L3) code is host-testable. The SDK wrappers need real symbols/FreeRTOS headers, so correctness is proven by the **compile-gate** `esp-gate/` — an ESP8266 SDK app whose `main/main.c` touches one symbol per lib; a clean link (`esp_libs_gate.elf`) means every L2/L3 symbol resolves against the SDK. `esp-libs/` doubles as an ESP8266 SDK component (`component.mk` at its root lists `COMPONENT_OBJS`); adding a new lib means wiring it into both `component.mk` and `esp-gate/main/main.c`.

```sh
# Needs a separately-installed ESP8266 RTOS SDK + xtensa toolchain (not vendored):
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
source ~/esp/idf-venv/bin/activate
cd esp-gate && make defconfig && make -j4   # → build/esp_libs_gate.elf (link-only; not flashed)
```

## Build — `avr-libs/` (ATmega328p)

Host-side: a committed `avr-libs/Makefile` builds + runs the **extracted pure logic** as host Unity tests (`make -C avr-libs test` / `strict`, reusing `avr-libs/third_party/unity`) — the `*_decode`/`*_conv`/`*_font`/`*_tick` files split out in the host-test cycles. The **AVR firmware** build has no committed makefiles — the AVR-Eclipse (`de.innot.avreclipse`) plugin generates them. MCU (`atmega328p`) and clock (`F_CPU=16000000UL`) come from each project's `.settings/de.innot.avreclipse.core.prefs`, not the source. **Only two folders are importable Eclipse projects** (full `.cproject`+`.settings`): `arduinoPinMapping` (project `my_avrCore`) and `uartRingbuff` (project `simple-UART-lib`). `adc/`, `encoder/`, `led7segShiftout/` carry only **empty stub** `.project` files; every other folder is loose source needing a fresh project.

```sh
# Eclipse: build the Debug/Release config of arduinoPinMapping or uartRingbuff → lib<ProjName>.a
# Manual build of any module:
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -I arduinoPinMapping -I ringBuffer -c <module>/*.c
avr-ar rcs lib<module>.a *.o
# Runnable firmware + flash (compile an example main, e.g. RTOS/test.c, ringBuffer/test.c):
avr-objcopy -O ihex -R .eeprom app.elf app.hex
avrdude -c arduino -p m328p -P /dev/ttyUSB0 -b 115200 -U flash:w:app.hex:i   # adjust programmer
```

Architecture spanning files: register-level drivers throughout, with `arduinoPinMapping` the only Arduino-style HAL (reused by `encoder`, `led7segShiftout`, `max6675`); `ringBuffer` is the shared FIFO primitive behind buffered `spi` and `uartRingbuff`. **Shared helpers are vendored (copied) per module** — `ringbuff.*`, `binary.h`, and the pin mapping appear in several folders, so a fix must be applied to each copy. Footguns: the SPI init symbol differs between display variants (`spiInitMaster` vs `spiInit`); `goot80_typeK` and `t33Soldering` encode *different* soldering-iron calibration curves behind the same `SOLDERING_H_` guard — never link both into one firmware.

## Build — `esp-idf-template/` (ESP32 family)

A version-agnostic ESP-IDF hello-world skeleton (`main/main.c` prints over serial). Needs a separately-installed ESP-IDF; source its env first (`. $IDF_PATH/export.sh`) — ESP-IDF is not vendored. Two build systems coexist: CMake/`idf.py` (v4.0+, preferred) and legacy GNU make (`Makefile` + `main/component.mk`, v3.x). The project name `app-template` is set in **both** `CMakeLists.txt` and `Makefile` (keep them in sync when renaming).

```sh
idf.py set-target esp32        # once (v4.0+)
idf.py menuconfig              # optional; writes gitignored sdkconfig
idf.py -p PORT flash monitor   # Ctrl-] to exit
# legacy v3.x: make menuconfig / make flash / make monitor
```

`main/CMakeLists.txt` uses the pre-v4.0 `COMPONENT_SRCS` + `register_component()` API — modernize to `idf_component_register(SRCS ... INCLUDE_DIRS ...)` on current IDF. `main/Kconfig.projbuild` advertises `ESP_WIFI_SSID`/`ESP_WIFI_PASSWORD` options that `main.c` does not use (leftover placeholders).
