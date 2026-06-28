#ifndef CLIBS_LCD1602_DEV_H
#define CLIBS_LCD1602_DEV_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lcd1602.h"

/* HD44780 16x2 char-LCD behind a PCF8574 I2C backpack, driven in 4-bit mode
 * over i2c_bus. addr is the 7-bit PCF8574 address (commonly 0x27 or 0x3F);
 * backlight tracks the P3 backlight bit applied to every byte. No malloc;
 * caller owns the struct. Not reentrant.
 *
 * NOT host-compiled — this glue pulls in esp_err.h / i2c_bus.h / rom/ets_sys.h
 * and is validated by the deferred xtensa COMPILE_GATE only. The pure nibble
 * framing it relies on (lcd1602_encode_byte) is host-tested in lcd1602.c. */
typedef struct {
    uint8_t addr;
    bool backlight;
} lcd1602;

/* Bind self to addr (e.g. 0x27), turn the backlight on, and run the HD44780
 * 4-bit init sequence (0x33, 0x32, 0x28, 0x0C, 0x06, 0x01). NULL self ->
 * ESP_ERR_INVALID_ARG. */
esp_err_t lcd1602_init(lcd1602 *self, uint8_t addr);

/* Send an instruction byte (RS=0). NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t lcd1602_command(lcd1602 *self, uint8_t cmd);

/* Write one character (RS=1). NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t lcd1602_putc(lcd1602 *self, char c);

/* Write a NUL-terminated string. NULL self/s -> ESP_ERR_INVALID_ARG. */
esp_err_t lcd1602_puts(lcd1602 *self, const char *s);

/* Move the cursor to (col, row): cmd 0x80 | ((row?0x40:0) + col).
 * NULL self -> ESP_ERR_INVALID_ARG. */
esp_err_t lcd1602_set_cursor(lcd1602 *self, uint8_t col, uint8_t row);

#endif /* CLIBS_LCD1602_DEV_H */
