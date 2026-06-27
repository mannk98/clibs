#ifndef CLIBS_BH1750_H
#define CLIBS_BH1750_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   /* NULL */

/* Pure (freestanding, host-testable) part of the BH1750 ambient-light driver.
 * No SDK headers here — the hardware glue over i2c_bus lives in bh1750_dev.{h,c}
 * and is COMPILE_GATE-only. */

/* Convert a 2-byte big-endian raw light reading to milli-lux.
 *
 * val      = ((uint32_t)raw[0] << 8) | raw[1]   (high byte first)
 * *milli_lux = val * 2500u / 3u                 (BH1750 H-res: 1 lux = 1.2 count,
 *                                                so lux = val / 1.2 = val * 2500/3000;
 *                                                in milli-lux that is val * 2500/3)
 *
 * The shift is on a non-negative uint32 only (no UB). Returns false and writes
 * nothing if raw or milli_lux is NULL; otherwise returns true. */
bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux);

#endif /* CLIBS_BH1750_H */
