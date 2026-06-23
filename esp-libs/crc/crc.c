#include "crc.h"

uint8_t crc8_maxim(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    if (data == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint8_t) ((crc >> 1) ^ 0x8C) : (uint8_t) (crc >> 1);
        }
    }
    return crc;
}

uint16_t crc16_modbus(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    if (data == NULL) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint16_t) ((crc >> 1) ^ 0xA001) : (uint16_t) (crc >> 1);
        }
    }
    return crc;
}
