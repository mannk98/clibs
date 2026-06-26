/*
 * max6675.h
 *
 *  Created on: Jul 11, 2022
 *      Author: mannk
 */

#ifndef MAX6675_H_
#define MAX6675_H_

#include <util/delay.h>
#include "wiring_digital.h"

struct MAX6675 {
	// public now but should turn to private somehow
	uint8_t sclk, miso, cs;
	uint8_t (*spiread)(const void *self);

	// public
	float (*readCel)(const void *self);
	float (*readFah)(const void *self);

};

typedef struct MAX6675 *max6675;

// new object
max6675 newMax6675(uint8_t SCLK, uint8_t CS, uint8_t MISO);

// method of it
float readCel(const max6675 self);
float readFah(const max6675 self);
// private
uint8_t spiread(const max6675 self);

/*
 * // method of it
float readCel(const void* self);
float readFah(const void* self);
// private
uint8_t spiread(const void* self);
 */
#endif /* MAX6675_H_ */
