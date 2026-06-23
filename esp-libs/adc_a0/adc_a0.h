#ifndef CLIBS_ADC_A0_H
#define CLIBS_ADC_A0_H

#include <stdint.h>
#include "esp_err.h"
#include "filter.h"   /* L1 ema */

/* Initialise the A0 (TOUT) ADC. */
esp_err_t adc_a0_init(void);
/* Read a raw sample (0..1023, unit 1/1023 V). NULL -> ESP_ERR_INVALID_ARG. */
esp_err_t adc_a0_read(uint16_t *raw);
/* Read a sample and feed it through the caller's EMA; *out gets the smoothed value. */
esp_err_t adc_a0_read_ema(ema *filter, int32_t *out);

#endif /* CLIBS_ADC_A0_H */
