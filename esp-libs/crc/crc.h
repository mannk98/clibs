#ifndef CLIBS_CRC_H
#define CLIBS_CRC_H

#include <stdint.h>
#include <stddef.h>

/* CRC-8/MAXIM (Dallas/Maxim 1-Wire): reflected poly 0x8C, init 0x00. For DS18B20 etc.
 * NULL data -> returns the init value 0x00. */
uint8_t crc8_maxim(const uint8_t *data, size_t len);
/* CRC-16/MODBUS: reflected poly 0xA001, init 0xFFFF. NULL data -> returns 0xFFFF. */
uint16_t crc16_modbus(const uint8_t *data, size_t len);

#endif /* CLIBS_CRC_H */
