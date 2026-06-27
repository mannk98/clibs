#include "ads1115_dev.h"
#include "i2c_bus.h"
#include "rom/ets_sys.h"   /* ets_delay_us */

/* First-cut conversion-ready wait. At the configured 128 SPS a conversion takes
 * ~7.8 ms; round up to 8 ms. Hardware-deferred: replace with the config-register
 * OS-bit poll once validated on a real device. */
#define ADS1115_CONV_DELAY_US 8000

esp_err_t ads1115_init(ads1115 *self, uint8_t addr, ads1115_pga pga)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    self->pga  = pga;
    return ESP_OK;
}

esp_err_t ads1115_read(ads1115 *self, uint8_t channel, int32_t *microvolts)
{
    if (self == NULL || microvolts == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Write the config register big-endian (high byte first). */
    uint16_t cfg = ads1115_config(channel, self->pga);
    uint8_t cfg_bytes[2] = { (uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF) };
    esp_err_t err = i2c_bus_write_reg(self->addr, ADS1115_REG_CONFIG,
                                      cfg_bytes, sizeof(cfg_bytes));
    if (err != ESP_OK) {
        return err;
    }

    /* Wait for the single-shot conversion to complete. */
    ets_delay_us(ADS1115_CONV_DELAY_US);

    /* Read the conversion register big-endian -> signed 16-bit. */
    uint8_t raw[2];
    err = i2c_bus_read_reg(self->addr, ADS1115_REG_CONVERSION, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    int16_t value = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);
    *microvolts = ads1115_to_microvolts(value, self->pga);
    return ESP_OK;
}
