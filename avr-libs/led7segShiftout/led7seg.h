/*
 * my74h595.h
 *
 *  Created on: Apr 5, 2022
 *      Author: mannk
 */
#ifndef MY74H595_H_
#define MY74H595_H_

#include <stdlib.h>
#include "binary.h"
#include "wiring_digital.h"

#define DEFAULT 0
#define CHOSE_MODE 1
#define TEMP_MATCH 2
#define SLEEP_MODE 3

typedef struct {
	uint8_t pinClk, pinLatch, pinData;
	uint16_t displayValue;
	uint8_t displayMode;
} sevenSeg;

void sevenSegInit(sevenSeg *self, uint8_t pinClk, uint8_t pinLatch,
		uint8_t pinData);
void sevenSegDisplay(sevenSeg *self, uint16_t value, uint8_t mode);

#endif /* MY74H595_H_ */
