#include "ds3231_dev.h"
#include "i2c_bus.h"

/* DS3231 register map. */
#define DS3231_REG_TIME 0x00  /* 7 timekeeping registers: 0x00..0x06 */
#define DS3231_REG_TEMP 0x11  /* 2 temperature registers: 0x11 (MSB), 0x12 (LSB) */

esp_err_t ds3231_init(ds3231 *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    return ESP_OK;
}

esp_err_t ds3231_get(ds3231 *self, ds3231_time *out)
{
    if (self == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[7];
    esp_err_t err = i2c_bus_read_reg(self->addr, DS3231_REG_TIME, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    if (!ds3231_decode(raw, out)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ds3231_set(ds3231 *self, const ds3231_time *t)
{
    if (self == NULL || t == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[7];
    if (!ds3231_encode(t, raw)) {
        return ESP_FAIL;
    }

    return i2c_bus_write_reg(self->addr, DS3231_REG_TIME, raw, sizeof(raw));
}

esp_err_t ds3231_read_temp(ds3231 *self, int32_t *milli_c)
{
    if (self == NULL || milli_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[2];
    esp_err_t err = i2c_bus_read_reg(self->addr, DS3231_REG_TEMP, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    *milli_c = ds3231_temp_mc(raw[0], raw[1]);
    return ESP_OK;
}
