#ifndef CLIBS_MAX6675_CONV_H
#define CLIBS_MAX6675_CONV_H

#include <stdint.h>

/* Returned by max6675_raw_to_celsius when the thermocouple is open (raw bit 2 set). */
#define MAX6675_OPEN (-100.0f)

/* Convert a raw 16-bit MAX6675 reading to degrees Celsius.
 * If bit 2 is set the thermocouple is open -> returns MAX6675_OPEN.
 * Otherwise the temperature is (raw >> 3) * 0.25 degC. */
float max6675_raw_to_celsius(uint16_t raw);

/* Standard Celsius -> Fahrenheit. */
float max6675_celsius_to_fahrenheit(float c);

#endif /* CLIBS_MAX6675_CONV_H */
