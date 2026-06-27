#include "spi_bus.h"

#include <string.h>

#include "driver/spi.h"

#define SPI_BUS_HOST   HSPI_HOST
#define SPI_BUS_MAXLEN 64

esp_err_t spi_bus_init(void)
{
    spi_config_t cfg = {0};
    cfg.interface.val = 0;
    cfg.interface.mosi_en = 1;
    cfg.interface.miso_en = 1;
    cfg.interface.cs_en = 0;   /* software CS per device (see spi_device) */
    cfg.intr_enable.val = 0;
    cfg.event_cb = NULL;
    cfg.mode = SPI_MASTER_MODE;
    cfg.clk_div = SPI_8MHz_DIV;
    return spi_init(SPI_BUS_HOST, &cfg);
}

esp_err_t spi_bus_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (tx == NULL || len == 0 || len > SPI_BUS_MAXLEN) {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t mosi[SPI_BUS_MAXLEN / 4] = {0};
    uint32_t miso[SPI_BUS_MAXLEN / 4] = {0};
    memcpy(mosi, tx, len);

    spi_trans_t trans = {0};
    trans.mosi = mosi;
    trans.bits.mosi = (uint32_t) (len * 8);
    if (rx != NULL) {
        trans.miso = miso;
        trans.bits.miso = (uint32_t) (len * 8);
    }
    esp_err_t err = spi_trans(SPI_BUS_HOST, &trans);
    if (err == ESP_OK && rx != NULL) {
        memcpy(rx, miso, len);
    }
    return err;
}

esp_err_t spi_device_init(spi_device *self, gpio_num_t cs_pin)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->cs_pin = cs_pin;
    esp_err_t err = gpio_set_direction(cs_pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) {
        return err;
    }
    return gpio_set_level(cs_pin, 1);   /* idle HIGH (active-low CS deasserted) */
}

esp_err_t spi_device_transfer(spi_device *self, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = gpio_set_level(self->cs_pin, 0);   /* assert CS */
    if (err != ESP_OK) {
        return err;
    }
    err = spi_bus_transfer(tx, rx, len);
    gpio_set_level(self->cs_pin, 1);                   /* always deassert CS */
    return err;
}
