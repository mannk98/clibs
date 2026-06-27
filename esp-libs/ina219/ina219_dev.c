#include "ina219_dev.h"
#include "i2c_bus.h"

/* INA219 register map. */
#define INA219_REG_CONFIG  0x00
#define INA219_REG_SHUNT   0x01  /* shunt voltage  */
#define INA219_REG_BUS     0x02  /* bus voltage    */
#define INA219_REG_POWER   0x03  /* power          */
#define INA219_REG_CURRENT 0x04  /* current        */
#define INA219_REG_CALIB   0x05  /* calibration    */

/* Config: 32 V bus range, /8 PGA (+-320 mV), 12-bit bus & shunt ADC,
 * shunt-and-bus continuous mode. */
#define INA219_CONFIG_DEFAULT 0x399F

/* All 16-bit registers are big-endian on the wire (MSB first). */
static esp_err_t ina219_write_reg16(uint8_t addr, uint8_t reg, uint16_t val)
{
    uint8_t buf[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return i2c_bus_write_reg(addr, reg, buf, sizeof(buf));
}

static esp_err_t ina219_read_reg16(uint8_t addr, uint8_t reg, uint16_t *out)
{
    uint8_t buf[2];
    esp_err_t err = i2c_bus_read_reg(addr, reg, buf, sizeof(buf));
    if (err != ESP_OK) {
        return err;
    }
    *out = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return ESP_OK;
}

esp_err_t ina219_init(ina219 *self, uint8_t addr,
                      uint32_t shunt_milliohm, uint32_t max_current_ma)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    self->current_lsb_ua = max_current_ma * 1000u / 32768u;

    esp_err_t err = ina219_write_reg16(addr, INA219_REG_CONFIG,
                                       INA219_CONFIG_DEFAULT);
    if (err != ESP_OK) {
        return err;
    }

    uint16_t cal = ina219_calibration(self->current_lsb_ua, shunt_milliohm);
    return ina219_write_reg16(addr, INA219_REG_CALIB, cal);
}

esp_err_t ina219_read_bus_mv(ina219 *self, int32_t *bus_mv)
{
    if (self == NULL || bus_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw;
    esp_err_t err = ina219_read_reg16(self->addr, INA219_REG_BUS, &raw);
    if (err != ESP_OK) {
        return err;
    }
    *bus_mv = ina219_bus_mv(raw);
    return ESP_OK;
}

esp_err_t ina219_read_shunt_uv(ina219 *self, int32_t *shunt_uv)
{
    if (self == NULL || shunt_uv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw;
    esp_err_t err = ina219_read_reg16(self->addr, INA219_REG_SHUNT, &raw);
    if (err != ESP_OK) {
        return err;
    }
    *shunt_uv = ina219_shunt_uv((int16_t)raw);
    return ESP_OK;
}

esp_err_t ina219_read_current_ua(ina219 *self, int32_t *current_ua)
{
    if (self == NULL || current_ua == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw;
    esp_err_t err = ina219_read_reg16(self->addr, INA219_REG_CURRENT, &raw);
    if (err != ESP_OK) {
        return err;
    }
    *current_ua = ina219_current_ua((int16_t)raw, self->current_lsb_ua);
    return ESP_OK;
}

esp_err_t ina219_read_power_uw(ina219 *self, int32_t *power_uw)
{
    if (self == NULL || power_uw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw;
    esp_err_t err = ina219_read_reg16(self->addr, INA219_REG_POWER, &raw);
    if (err != ESP_OK) {
        return err;
    }
    *power_uw = ina219_power_uw(raw, self->current_lsb_ua);
    return ESP_OK;
}
