/*
 * T33Soldering.c — T33/type-C style soldering-iron thermocouple ADC->degC curve.
 */
#include "T33Soldering.h"

/* thermocouple voltage breakpoints (mV) */
#define VOLTAGE_AT_190 2.919
#define VOLTAGE_AT_270 4.321
#define VOLTAGE_AT_350 5.789

/* per-band offsets from the op-amp configuration */
#define OFFSET_under190 25
#define OFFSET_190to270 32
#define OFFSET_270to350 23
#define OFFSET_over350  19

float t33_vol_to_temp(uint16_t adc_value) {
    float realTemp = 0;
    float thermoVoltage = adc_value * 0.00716; /* mV */
    if (thermoVoltage == 0)
        return 0; /* not connected */

    if (thermoVoltage > 0 && thermoVoltage <= VOLTAGE_AT_190)
        realTemp = 62.98 * thermoVoltage + 7.74 + OFFSET_under190;
    else if (thermoVoltage > VOLTAGE_AT_190 && thermoVoltage <= VOLTAGE_AT_270)
        realTemp = 57.07 * thermoVoltage + 23.74 + OFFSET_190to270;
    else if (thermoVoltage > VOLTAGE_AT_270 && thermoVoltage <= VOLTAGE_AT_350)
        realTemp = 54.47 * thermoVoltage + 34.88 + OFFSET_270to350;
    else if (thermoVoltage > VOLTAGE_AT_350)
        realTemp = 52.77 * thermoVoltage + 44.68 + OFFSET_over350;

    return realTemp;
}
