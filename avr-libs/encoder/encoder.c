/*
 * my_encoder.c
 *
 *  Created on: Apr 1, 2022
 *      Author: mannk
 */

#include "encoder.h"
#include "encoder_decode.h"

//uint16_t counter;

void encoderInit(encoder *self, uint8_t pinA, uint8_t pinB) {

	self->pinA = pinA;
	self->pinB = pinB;
	self->count = 0;
	pinMode(pinA, INPUT_PULLUP);
	pinMode(pinB, INPUT_PULLUP);
	self->pre_pinA_State = digitalRead(self->pinA);

}

void encoderCheck(encoder *self, void *counter) { // pass address of itself
	uint8_t pinA_State = digitalRead(self->pinA);
	uint8_t pinB_State = digitalRead(self->pinB);

	encoder_decode(&self->pre_pinA_State, &self->count,
	               pinA_State, pinB_State, (int *)counter);
}

