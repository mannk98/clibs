#include "bme280.h"

#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Register map. */
#define BME280_REG_ID        0xD0
#define BME280_REG_RESET     0xE0
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_DATA      0xF7   /* press MSB; 8 bytes through hum LSB (0xFE) */
#define BME280_REG_CALIB00   0x88   /* 0x88..0xA1 -> 26 bytes */
#define BME280_REG_CALIB26   0xE1   /* 0xE1..0xE7 -> 7 bytes  */

#define BME280_CHIP_ID       0x60
#define BME280_RESET_WORD    0xB6

/* ctrl_meas: osrs_t x1 (001), osrs_p x1 (001), mode forced (01) = 0b00100101. */
#define BME280_CTRL_MEAS_FORCED 0x25
/* ctrl_hum: osrs_h x1. */
#define BME280_CTRL_HUM_X1      0x01
/* status: measuring bit. */
#define BME280_STATUS_MEASURING 0x08

esp_err_t bme280_init(bme280 *self, uint8_t addr)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->addr = addr;

    uint8_t id = 0;
    esp_err_t err = i2c_bus_read_reg(addr, BME280_REG_ID, &id, 1);
    if (err != ESP_OK) {
        return err;
    }
    if (id != BME280_CHIP_ID) {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t reset = BME280_RESET_WORD;
    err = i2c_bus_write_reg(addr, BME280_REG_RESET, &reset, 1);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(5));   /* start-up time after reset */

    uint8_t calib26[26];
    uint8_t calibH[7];
    err = i2c_bus_read_reg(addr, BME280_REG_CALIB00, calib26, sizeof(calib26));
    if (err != ESP_OK) {
        return err;
    }
    err = i2c_bus_read_reg(addr, BME280_REG_CALIB26, calibH, sizeof(calibH));
    if (err != ESP_OK) {
        return err;
    }
    if (!bme280_parse_calib(calib26, calibH, &self->calib)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* ctrl_hum must be written before ctrl_meas for it to take effect. */
    uint8_t ch = BME280_CTRL_HUM_X1;
    err = i2c_bus_write_reg(addr, BME280_REG_CTRL_HUM, &ch, 1);
    if (err != ESP_OK) {
        return err;
    }
    return ESP_OK;
}

esp_err_t bme280_read(bme280 *self, bme280_reading *out)
{
    if (self == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Trigger a single forced-mode conversion. */
    uint8_t cm = BME280_CTRL_MEAS_FORCED;
    esp_err_t err = i2c_bus_write_reg(self->addr, BME280_REG_CTRL_MEAS, &cm, 1);
    if (err != ESP_OK) {
        return err;
    }

    /* Poll the measuring bit until the conversion completes (bounded). */
    for (int i = 0; i < 10; i++) {
        uint8_t status = 0;
        err = i2c_bus_read_reg(self->addr, BME280_REG_STATUS, &status, 1);
        if (err != ESP_OK) {
            return err;
        }
        if ((status & BME280_STATUS_MEASURING) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t d[8];
    err = i2c_bus_read_reg(self->addr, BME280_REG_DATA, d, sizeof(d));
    if (err != ESP_OK) {
        return err;
    }

    /* 20-bit pressure/temperature (MSB,LSB,XLSB), 16-bit humidity (MSB,LSB). */
    int32_t adc_P = ((int32_t) d[0] << 12) | ((int32_t) d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t) d[3] << 12) | ((int32_t) d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t) d[6] << 8) | d[7];

    if (!bme280_compensate(&self->calib, adc_T, adc_P, adc_H, out)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}
