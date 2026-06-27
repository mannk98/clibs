#include "sht3x_dev.h"
#include "i2c_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Single-shot, high-repeatability, clock-stretching disabled: command 0x2400. */
#define SHT3X_CMD_MEAS_HIGH_MSB 0x24
#define SHT3X_CMD_MEAS_HIGH_LSB 0x00

/* High-repeatability measurement duration (datasheet max ~15 ms). */
#define SHT3X_MEAS_DELAY_MS 15

esp_err_t sht3x_init(sht3x *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;
    return ESP_OK;
}

esp_err_t sht3x_read(sht3x *self, int32_t *temp_mc, int32_t *humi_mrh)
{
    if (self == NULL || temp_mc == NULL || humi_mrh == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t cmd[2] = { SHT3X_CMD_MEAS_HIGH_MSB, SHT3X_CMD_MEAS_HIGH_LSB };
    esp_err_t err = i2c_bus_write(self->addr, cmd, sizeof(cmd));
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(SHT3X_MEAS_DELAY_MS));

    uint8_t raw[6];
    err = i2c_bus_read(self->addr, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    if (!sht3x_parse(raw, temp_mc, humi_mrh)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
