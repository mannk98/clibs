# clibs/common

Portable, **malloc-free** C libraries owned by the clibs workspace, written to run
on microcontrollers (ATmega/AVR, ESP) as well as on a host PC. Each library is a
transparent struct + `module_<verb>(self, …)` methods over caller-provided
storage — deterministic RAM, no heap. The reusable libs depend only on
`<stdint.h>`, `<stdbool.h>`, `<stddef.h>` (+`<string.h>` for `memcpy`), so they
cross-compile for AVR unchanged.

## Libraries

| Library     | Path          | What it is |
|-------------|---------------|------------|
| `dll`       | `dll/`        | Doubly linked list of `void*` over a caller node pool (free-list, no malloc). |
| `ringbuf`   | `ringbuf/`    | Generic fixed-capacity FIFO over a caller array; value-copy semantics; optional overwrite-oldest. |
| `str_utils` | `str_utils/`  | Freestanding string helpers (`str_len`, `str_compare_n`, `str_cpy_n`, `str_reverse`). |
| `log`       | `log/`        | Leveled logger — fork of [rxi/log.c](https://github.com/rxi/log.c) with a per-message `tag`. Hosted by design; on MCU register a callback that writes to UART. |
| `bitset`    | `bitset/`     | Bit array over a caller `uint32_t[]` (set/clear/toggle/test/count/find-first-set). |
| `mempool`   | `mempool/`    | Fixed-block allocator (alloc/free fixed-size blocks from a caller buffer) — the malloc-free building block. |
| `fixed`     | `fixed/`      | Q16.16 fixed-point value type (mul/div/sqrt + int/float conversion, no FPU). |
| `crc`       | `crc/`        | Canonical checksums: `crc8_maxim`, `crc16_ccitt`, `crc16_modbus`, `crc32` (stateless functions). |
| `fsm`       | `fsm/`        | Table-driven finite state machine (states × events → action + next state). |

`crc` (stateless functions) and `fixed` (a value type like `int`) are not
`self`-objects — the OOP struct+method convention applies only to the stateful
libs (`dll`, `ringbuf`, `bitset`, `mempool`, `fsm`). `crc` is the canonical
home for these checksums; `esp-libs/crc` will be de-duped to re-export it later.

## Reusing a library

Copy the library's `.c` + `.h` into your project (e.g. add `dll/dll.c` and
`#include "dll.h"`). Provide your own storage:

```c
dll_node pool[16];
dll_list list;
dll_init(&list, pool, 16);
int v = 42;
dll_push_tail(&list, &v);
```

The containers are **not reentrant**; if you share one with an ISR, guard the
calls with `ATOMIC_BLOCK`/`cli`.

## Tests

Host unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored
under `third_party/unity/`).

```sh
make test     # build + run all suites (str_utils, ringbuf, dll, log, bitset, mempool, fixed, crc, fsm)
make strict   # warning-clean compile gate (-Werror) for the reusable libs
make clean
```

## License

MIT — see `LICENSE`. `log/` retains its upstream rxi MIT copyright.
