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
    cfg.interface.cs_en = 1;
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
