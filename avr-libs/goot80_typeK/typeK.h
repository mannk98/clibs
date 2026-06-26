/*
 * typeK.h — GOOT-80 style soldering-iron thermocouple ADC->degC curve.
 */
#ifndef CLIBS_GOOT80_TYPEK_H
#define CLIBS_GOOT80_TYPEK_H

#include <stdint.h>

/* Convert a raw 10-bit ADC reading to temperature (degC) for the GOOT-80 iron.
 * Returns 0 for adc_value == 0 (treated as not-connected). */
float goot80_vol_to_temp(uint16_t adc_value);

#endif /* CLIBS_GOOT80_TYPEK_H */
