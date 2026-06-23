#ifndef CLIBS_DS18B20_PARSE_H
#define CLIBS_DS18B20_PARSE_H

#include <stdint.h>
#include <stdbool.h>

/* Validate the 9-byte DS18B20 scratchpad (crc8_maxim(scratch,8) == scratch[8]) and
 * convert the temperature (scratch[0]=LSB, scratch[1]=MSB; LSB = 1/16 C) to
 * milli-degrees C. Bad CRC / NULL -> false. Pure: no SDK headers (host-testable). */
bool ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc);

#endif /* CLIBS_DS18B20_PARSE_H */
