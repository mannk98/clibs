# avr-libs

Pure-C, bare-metal peripheral and utility libraries for the **Atmel ATmega328p**
(Arduino Uno / Nano class MCU). A personal learning/hobby toolkit: register-level
drivers, data structures, display and sensor drivers, a PID controller,
Arduino-style timekeeping, and a small cooperative RTOS — each as an independent
AVR-Eclipse static-library project.

> Consumed as a git submodule of [`clibs`](https://github.com/mannk98/clibs)
> (`origin: git@mannk98:mannk98/avr-libs.git`, branch `main`).

## Target hardware & toolchain

- **MCU:** ATmega328p
- **Clock (`F_CPU`):** 16 MHz (`16000000UL`)
- **Toolchain:** `avr-gcc` / `avr-libc` (WinAVR) via **Eclipse CDT + AVR-Eclipse
  plugin** (`de.innot.avreclipse`)
- **Artifact:** each project builds a static library (`lib<Project>.a`)
- **Optimization:** `-Os` (size) in Release; debug info in Debug

The MCU type and clock are not hard-coded in most sources — they come from each
project's `.settings/de.innot.avreclipse.core.prefs` (`MCUType=atmega328p`,
`ClockFrequency=16000000`), which Eclipse passes as
`-mmcu=atmega328p -DF_CPU=16000000UL`. (A few files such as `uartRingbuff/uart.h`
also `#define F_CPU 16000000UL` directly.)

## Building

There are **no committed makefiles or `Debug/` folders** — the AVR-Eclipse plugin
generates them at build time.

### A. In Eclipse (intended path)

1. Install Eclipse CDT with the AVR-Eclipse (`de.innot.avreclipse`) plugin.
2. **Only two folders are importable as existing projects** (they ship a full
   `.cproject` + `.settings`):
   - `arduinoPinMapping` → project **`my_avrCore`**
   - `uartRingbuff` → project **`simple-UART-lib`**

   Build a Debug/Release configuration → produces `libmy_avrCore.a` /
   `libsimple-UART-lib.a`.
3. Every other folder ships **source only**. `adc/`, `encoder/`,
   `led7segShiftout/` contain empty stub `.project` files (not buildable as-is);
   the rest have no project metadata. To build any of them, create a new
   *AVR Cross Target Static Library* project (set MCU = `atmega328p`,
   F_CPU = `16000000`) and add the folder's `.c`/`.h`.

### B. Command line (any module)

```sh
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os \
        -I arduinoPinMapping -I ringBuffer -c <module>/*.c
avr-ar rcs lib<module>.a *.o
```

### Building runnable firmware + flashing

Compile a `main.c` (see `RTOS/test.c`, `uartRingbuff/test.c`
for example mains) together with the module sources, then:

```sh
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -o app.elf main.c <sources>.c
avr-objcopy -O ihex -R .eeprom app.elf app.hex
avrdude -c arduino -p m328p -P /dev/ttyUSB0 -b 115200 -U flash:w:app.hex:i   # adjust programmer
```

(No avrdude/programmer config is committed; the line above is the standard 328p
invocation — adjust `-c`/`-P`/`-b` for your programmer.)

## Modules

### Communication & buffering
- **uartRingbuff/** — interrupt-driven UART0 with a TX ring buffer (`uart_init`,
  `uart_putChar/putString/putInteger`, drained in `USART_UDRE_vect`). Subfolder
  `uart_simple/` is a simpler polling UART.
- **spi/** — hardware SPI master: `spiTradeByte` (blocking) and `spiTradeByteISR`
  (interrupt-driven via `SPI_STC_vect` + ring buffer).
- **ringBuffer/** — malloc-free byte FIFO over caller-provided storage (`ringbuf_*`
  API: `ringbuf_init/put/get/peek/put_string/get_bytes`); the shared primitive
  behind buffered UART and SPI.

### Analog & sensors
- **adc/** — 10-bit / 8-bit ADC, polled 8-sample average (`adc_singeRead`) or
  free-running ISR averaging (`adc_isrStartRead`/`adc_isrReturn`).
- **max6675/** — MAX6675 type-K thermocouple reader (bit-banged 3-wire),
  0.25 °C resolution, open-thermocouple detection. OOP-in-C (`newMax6675`).
- **goot80_typeK/** — ADC→°C piecewise-linear curve for a GOOT-80 style soldering
  iron (`goot80_vol_to_temp`).
- **t33Soldering/** — ADC→°C curve for a T33/type-C style iron (`t33_vol_to_temp`;
  different calibration; do not link together with `goot80_typeK`).

### Timing & control
- **timer/** — Timer1 16-bit Fast PWM on OC1A/OC1B (`initPwm_oc1a/oc1b`,
  `updatePwm_*`, stop/resume).
- **wallClock/** — Arduino-style `millis()`/`micros()` on Timer0 overflow.
- **myPid/** — floating-point PID controller with anti-windup and band-limited
  derivative (no AVR deps).

### GPIO, input & display
- **arduinoPinMapping/** — Arduino-style digital HAL (`pinMode`/`digitalWrite`/
  `digitalRead`/`pinTongle`) with PROGMEM pin tables; reused by several modules.
- **encoder/** — polled quadrature rotary-encoder decoder updating a
  caller-supplied counter.
- **led7segShiftout/** — 4-digit 7-segment via 74HC595, **bit-banged** GPIO
  (~2.5 ms / refresh).
- **led7segSpi/** — same display over **hardware SPI**, polled or ISR
  (`sevenSegIsrDisplay`) (~512 µs / refresh).
- **led7segSpi(noISR)/** — SPI polling variant taking a delay-callback for
  multiplexing (uses `spiInit`, not `spiInitMaster`).

### RTOS
- **RTOS/** — `sonRTOS`, a priority-based cooperative kernel (setjmp/longjmp
  context switch, 1 ms Timer2 tick, up to 8 tasks + 8 mutexes). API:
  `osWaitTicks/osWaitMs`, `osYield`, `osSuspend`, `osGetMutex/osReleaseMutex`.
  See `RTOS/test.c` for a blink + UART demo.

## Testing

The pure-logic modules (`myPid`, `goot80_typeK`, `t33Soldering`, `ringBuffer`,
`max6675` (conv), `encoder` (decode), `wallClock` (tick), `led7seg_font`) have
host Unity test suites. Three gates are available:

```sh
make test    # build + run all eight suites (pid 5, goot80 6, t33 6, ringbuf 12, max6675 3, encoder 3, wallclock 3, led7seg 2 tests)
make strict  # recompile each module with -Werror (no warnings allowed)
make avr     # cross-compile each module with avr-gcc as AVR target check
             # AVR_CC defaults to /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc
```

All three gates must report `OK` before merging changes to these modules.

## Conventions & notes

- One peripheral/feature per folder; each is a standalone static-library project.
- Mostly **register-level** code; `arduinoPinMapping` is the only HAL layer.
- ISRs guard shared state with `ATOMIC_BLOCK` or
  `oldSREG = SREG; cli(); …; SREG = oldSREG`.
- **Shared helpers are vendored (copied) per module** — `ringbuff.*`, `binary.h`,
  and the Arduino pin mapping appear in multiple folders, so fixes must be applied
  to each copy. Exception: the **7-segment font is now shared** (`led7seg_font/`);
  all three display variants call `led7seg_segments()` from that single module
  rather than each owning a private copy.
- Mixed styles: OOP-in-C with function pointers (`max6675`) vs. `self`-pointer
  free functions (`encoder`, `myPid`, `led7seg*`, RTOS).
- This is hobby-grade code with known rough edges — e.g. the SPI init symbol
  differs between display variants. (The old `ringBuffer` `#define uint8_t char`
  hack was fixed in cycle 2a; the driver-vendored `ringbuff` copies still carry
  the old API until the driver cycle.)

## Repository status

- Git submodule of `clibs`; tracked at commit `42a649e` on `main`.
- The `.metadata/` directory is Eclipse workspace state (CDT/EGit), not source.
