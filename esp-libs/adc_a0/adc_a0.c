#include "adc_a0.h"

#include "driver/adc.h"

esp_err_t adc_a0_init(void)
{
    adc_config_t cfg = { .mode = ADC_READ_TOUT_MODE, .clk_div = 8 };
    return adc_init(&cfg);
}

esp_err_t adc_a0_read(uint16_t *raw)
{
    if (raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_read(raw);
}

esp_err_t adc_a0_read_ema(ema *filter, int32_t *out)
{
    if (filter == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw = 0;
    esp_err_t err = adc_read(&raw);
    if (err != ESP_OK) {
        return err;
    }
    *out = ema_update(filter, (int32_t) raw);
    return ESP_OK;
}
