#ifndef CLIBS_CRC_H
#define CLIBS_CRC_H

/*
 * crc — canonical stateless CRC functions (no self, no struct).
 *
 * Bytewise (table-free, flash-light), freestanding portable C.
 * `data == NULL` or `len == 0` returns the CRC value over zero bytes
 * (the algorithm's init/final-xor result, treating NULL as zero bytes).
 *
 * Not reentrant in the sense of shared mutable state? No — these are pure
 * functions with no global state, safe to call from any context.
 */

#include <stdint.h>
#include <stddef.h>

/* CRC-8/MAXIM (Dallas/1-Wire): poly 0x8C reflected, init 0x00, no xorout.
 * Check: crc8_maxim("123456789", 9) == 0xA1. Empty/NULL -> 0x00. */
uint8_t crc8_maxim(const void *data, size_t len);

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflect, no xorout.
 * Check: crc16_ccitt("123456789", 9) == 0x29B1. Empty/NULL -> 0xFFFF. */
uint16_t crc16_ccitt(const void *data, size_t len);

/* CRC-16/MODBUS: poly 0xA001 reflected, init 0xFFFF, no xorout.
 * Check: crc16_modbus("123456789", 9) == 0x4B37. Empty/NULL -> 0xFFFF. */
uint16_t crc16_modbus(const void *data, size_t len);

/* CRC-32 (zlib/PKZIP): poly 0xEDB88320 reflected, init/xorout 0xFFFFFFFF.
 * Check: crc32("123456789", 9) == 0xCBF43926. Empty/NULL -> 0x00000000. */
uint32_t crc32(const void *data, size_t len);

#endif /* CLIBS_CRC_H */
