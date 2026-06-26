/*
 * typeK.c — GOOT-80 style soldering-iron thermocouple ADC->degC curve.
 */
#include "typeK.h"

/* thermocouple voltage breakpoints (mV) */
#define VOLTAGE_AT_190 7.739
#define VOLTAGE_AT_270 10.971
#define VOLTAGE_AT_350 14.293

float goot80_vol_to_temp(uint16_t adc_value) {
    float realTemp = 0;
    float thermoVoltage = adc_value * 0.02237; /* mV */
    if (thermoVoltage == 0)
        return 0; /* not connected */

    if (thermoVoltage > 0 && thermoVoltage <= VOLTAGE_AT_190)
        realTemp = 24.95 * thermoVoltage - 3.12;
    else if (thermoVoltage > VOLTAGE_AT_190 && thermoVoltage <= VOLTAGE_AT_270)
        realTemp = 24.7 * thermoVoltage - 0.97;
    else if (thermoVoltage > VOLTAGE_AT_270 && thermoVoltage <= VOLTAGE_AT_350)
        realTemp = 24.1 * thermoVoltage + 9.67;
    else if (thermoVoltage > VOLTAGE_AT_350)
        realTemp = 23.8 * thermoVoltage + 44.68;

    return realTemp;
}
