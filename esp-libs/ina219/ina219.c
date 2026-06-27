#include "ina219.h"

int32_t ina219_bus_mv(uint16_t raw)
{
    return (int32_t)((uint32_t)(raw >> 3) * 4u);
}

int32_t ina219_shunt_uv(int16_t raw)
{
    return (int32_t)raw * 10;
}

int32_t ina219_current_ua(int16_t raw, uint32_t current_lsb_ua)
{
    return (int32_t)((int64_t)raw * (int64_t)current_lsb_ua);
}

int32_t ina219_power_uw(uint16_t raw, uint32_t current_lsb_ua)
{
    return (int32_t)((int64_t)raw * 20 * (int64_t)current_lsb_ua);
}

uint16_t ina219_calibration(uint32_t current_lsb_ua, uint32_t shunt_milliohm)
{
    uint64_t denom = (uint64_t)current_lsb_ua * shunt_milliohm;
    return denom == 0 ? 0u : (uint16_t)(40960000ULL / denom);
}
