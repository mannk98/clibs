#include "i2c_bus.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"   /* pdMS_TO_TICKS */

#define I2C_BUS_PORT    I2C_NUM_0
#define I2C_BUS_TIMEOUT pdMS_TO_TICKS(1000)

esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_stretch_tick = 300,
    };
    esp_err_t err = i2c_param_config(I2C_BUS_PORT, &cfg);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_BUS_PORT, I2C_MODE_MASTER);
}

esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (data == NULL && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0) {
        i2c_master_write(cmd, (uint8_t *) data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);   /* repeated start */
    i2c_master_write_byte(cmd, (uint8_t) ((addr << 1) | I2C_MASTER_READ), true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_BUS_PORT, cmd, I2C_BUS_TIMEOUT);
    i2c_cmd_link_delete(cmd);
    return err;
}
