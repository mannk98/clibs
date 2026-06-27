#ifndef CLIBS_ESP_LIBS_CRC_H
#define CLIBS_ESP_LIBS_CRC_H

/* Re-export shim: esp-libs/crc now forwards to the canonical common/crc.
 *
 * crc8_maxim + crc16_modbus are byte-for-byte identical to the former local
 * copy (same reflected polys/inits); common/crc additionally exposes
 * crc16_ccitt + crc32. Consumers keep `#include "crc.h"` unchanged — the only
 * visible difference is the parameter type widened to `const void *` (a
 * `const uint8_t *` argument converts implicitly, so existing call sites such
 * as ds18b20 / esp-gate are unaffected).
 *
 * NOTE: a DISTINCT guard (CLIBS_ESP_LIBS_CRC_H) is used here on purpose — the
 * common header guards with CLIBS_CRC_H, so defining that here would suppress
 * common's declarations. The relative include resolves to clibs/common/crc/. */
#include "../../common/crc/crc.h"

#endif /* CLIBS_ESP_LIBS_CRC_H */
