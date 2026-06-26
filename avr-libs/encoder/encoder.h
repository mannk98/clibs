/*
 * my_encoder.h
 *
 *  Created on: Apr 1, 2022
 *      Author: mannk
 */

#ifndef MY_ENCODER_H_
#define MY_ENCODER_H_

#include <avr/io.h>
#include "arduinoPinMapping/wiring_digital.h"

typedef struct {
	uint8_t pinA, pinB, pre_pinA_State, count;
} encoder;

void encoderInit(encoder *self, uint8_t pinA, uint8_t pinB);
void encoderCheck(encoder *self, void *counter);

#endif /* MY_ENCODER_H_ */

