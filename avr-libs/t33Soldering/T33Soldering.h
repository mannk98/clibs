/*
 * T33Soldering.h — T33/type-C style soldering-iron thermocouple ADC->degC curve.
 */
#ifndef CLIBS_T33_SOLDERING_H
#define CLIBS_T33_SOLDERING_H

#include <stdint.h>

/* Convert a raw 10-bit ADC reading to temperature (degC) for the T33 iron.
 * Returns 0 for adc_value == 0 (treated as not-connected). */
float t33_vol_to_temp(uint16_t adc_value);

#endif /* CLIBS_T33_SOLDERING_H */
