#include "max7219.h"
#include "spi_bus.h"

#define MAX7219_REG_DECODE   0x09
#define MAX7219_REG_INTENS   0x0A
#define MAX7219_REG_SCANLIM  0x0B
#define MAX7219_REG_SHUTDOWN 0x0C
#define MAX7219_REG_TEST     0x0F

static esp_err_t max7219_send(max7219 *self, uint8_t reg, uint8_t data) {
    uint8_t frame[2];
    max7219_frame(reg, data, frame);
    return spi_device_transfer(&self->dev, frame, NULL, 2);
}

esp_err_t max7219_init(max7219 *self, gpio_num_t cs_pin, uint8_t intensity) {
    if (!self) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t e = spi_device_init(&self->dev, cs_pin);
    if (e != ESP_OK) {
        return e;
    }
    self->intensity = intensity;
    if ((e = max7219_send(self, MAX7219_REG_TEST, 0x00)) != ESP_OK) {        /* display test off */
        return e;
    }
    if ((e = max7219_send(self, MAX7219_REG_DECODE, 0x00)) != ESP_OK) {      /* no-decode (raw segments) */
        return e;
    }
    if ((e = max7219_send(self, MAX7219_REG_SCANLIM, 0x07)) != ESP_OK) {     /* scan all 8 digits */
        return e;
    }
    if ((e = max7219_send(self, MAX7219_REG_INTENS, (uint8_t)(intensity & 0x0Fu))) != ESP_OK) {
        return e;
    }
    return max7219_send(self, MAX7219_REG_SHUTDOWN, 0x01);                   /* normal operation */
}

esp_err_t max7219_set_digit(max7219 *self, uint8_t pos, uint8_t value, bool dp) {
    if (!self || pos > 7u) {
        return ESP_ERR_INVALID_ARG;
    }
    return max7219_send(self, (uint8_t)(0x01u + pos), max7219_digit_segments(value, dp));
}

esp_err_t max7219_set_row(max7219 *self, uint8_t row, uint8_t bits) {
    if (!self || row > 7u) {
        return ESP_ERR_INVALID_ARG;
    }
    return max7219_send(self, (uint8_t)(0x01u + row), bits);
}
