#include "bh1750_dev.h"
#include "i2c_bus.h"

/* BH1750 opcodes. */
#define BH1750_OP_CONT_HRES 0x10  /* continuous H-resolution mode */

esp_err_t bh1750_init(bh1750 *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;

    const uint8_t op = BH1750_OP_CONT_HRES;
    return i2c_bus_write(self->addr, &op, 1);
}

esp_err_t bh1750_read(bh1750 *self, uint32_t *milli_lux)
{
    if (self == NULL || milli_lux == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[2];
    esp_err_t err = i2c_bus_read(self->addr, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    if (!bh1750_parse(raw, milli_lux)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
