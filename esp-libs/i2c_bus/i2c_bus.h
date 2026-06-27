#ifndef CLIBS_I2C_BUS_H
#define CLIBS_I2C_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"   /* gpio_num_t */

/* Configure I2C port 0 as master on the given pins (internal pull-ups on). */
esp_err_t i2c_bus_init(gpio_num_t sda, gpio_num_t scl);
/* Write `len` bytes to register `reg` of the 7-bit `addr` device. data may be NULL iff len==0. */
esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
/* Read `len` bytes from register `reg` of the 7-bit `addr` device (repeated-start). */
esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

/* Raw write: START, addr+W, data[0..len-1], STOP. No register/pointer byte.
 * data may be NULL iff len == 0. For command-based devices (BH1750, SHT3x). */
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);

/* Raw read: START, addr+R, read len bytes (last NACK), STOP. No register write first.
 * data != NULL, len > 0. For command-based devices (BH1750, SHT3x). */
esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len);

#endif /* CLIBS_I2C_BUS_H */
