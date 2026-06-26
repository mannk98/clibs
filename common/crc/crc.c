#include "crc.h"

/* CRC-8/MAXIM: poly 0x8C (reflected 0x31), init 0x00, no xorout. */
uint8_t crc8_maxim(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint8_t crc = 0x00u;
    if (p == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (int b = 0; b < 8; ++b) {
            crc = (uint8_t)((crc & 1u) ? ((crc >> 1) ^ 0x8Cu) : (crc >> 1));
        }
    }
    return crc;
}

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflect, no xorout. */
uint16_t crc16_ccitt(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint16_t crc = 0xFFFFu;
    if (p == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (int b = 0; b < 8; ++b) {
            crc = (uint16_t)((crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1));
        }
    }
    return crc;
}

/* CRC-16/MODBUS: poly 0xA001 (reflected 0x8005), init 0xFFFF, no xorout. */
uint16_t crc16_modbus(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint16_t crc = 0xFFFFu;
    if (p == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)p[i];
        for (int b = 0; b < 8; ++b) {
            crc = (uint16_t)((crc & 1u) ? ((crc >> 1) ^ 0xA001u) : (crc >> 1));
        }
    }
    return crc;
}

/* CRC-32 (zlib/PKZIP): poly 0xEDB88320 (reflected), init/xorout 0xFFFFFFFF. */
uint32_t crc32(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    if (p == NULL) {
        return crc ^ 0xFFFFFFFFu;
    }
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint32_t)p[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
