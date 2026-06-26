/*
 * 7seg_74hc595.c
 *
 *  Created on: Apr 5, 2022
 *      Author: mannk
 */

#include "led7seg.h"
#include "led7seg_font.h"

#define LSBFIRST 1
#define RIBFIRST 0

static void shiftOut(sevenSeg *self, uint8_t bitOrder, uint8_t val); // static: range of it: inside Led7SEG.c

static void shiftOut(sevenSeg *self, uint8_t bitOrder, uint8_t val) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (bitOrder == LSBFIRST)
			digitalWrite(self->pinData, !!(val & (1 << i)));
		else
			digitalWrite(self->pinData, !!(val & (1 << (7 - i))));

		digitalWrite(self->pinClk, HIGH);
		digitalWrite(self->pinClk, LOW);
	}
}

void sevenSegInit(sevenSeg *self, uint8_t pinClk, uint8_t pinLatch,
		uint8_t pinData) {

	self->pinClk = pinClk;
	self->pinLatch = pinLatch;
	self->pinData = pinData;
	self->displayValue = 0;

	pinMode(self->pinClk, OUTPUT);
	pinMode(self->pinLatch, OUTPUT);
	pinMode(self->pinData, OUTPUT);
}

// 321	3 -> 2 -> 1, right -> B00001000 <- left	// this funtion and old shiftout(use digitalWrite) take 2.5ms for 4digit display
void sevenSegDisplay(sevenSeg *self, uint16_t value, uint8_t mode) {
	uint8_t common = B00001000;

	if (mode == SLEEP_MODE) {
		while (common) {
			digitalWrite(self->pinLatch, LOW);
			shiftOut(self, RIBFIRST, common);
			shiftOut(self, RIBFIRST, B10111111);
			digitalWrite(self->pinLatch, HIGH);
			common >>= 1;
		}
	} else if (mode == DEFAULT) {
		if (value == 0) {
			digitalWrite(self->pinLatch, LOW);
			shiftOut(self, RIBFIRST, common);
			shiftOut(self, RIBFIRST, led7seg_segments(value));
			digitalWrite(self->pinLatch, HIGH);
		} else {
			while (value) {
				digitalWrite(self->pinLatch, LOW);
				shiftOut(self, RIBFIRST, common);
				shiftOut(self, RIBFIRST, led7seg_segments(value % 10));
				digitalWrite(self->pinLatch, HIGH);

				common = common >> 1;
				value /= 10;
			}
		}
	} else if (mode == CHOSE_MODE) {
// display _
		digitalWrite(self->pinLatch, LOW);
		shiftOut(self, RIBFIRST, B00000001);
		shiftOut(self, RIBFIRST, B11110111);
		digitalWrite(self->pinLatch, HIGH);

		while (value) {
			digitalWrite(self->pinLatch, LOW);
			shiftOut(self, RIBFIRST, common);
			shiftOut(self, RIBFIRST, led7seg_segments(value % 10));
			digitalWrite(self->pinLatch, HIGH);

			common = common >> 1;
			value /= 10;
		}

	} else if (mode == TEMP_MATCH) {
// display Dot
		digitalWrite(self->pinLatch, LOW);
		shiftOut(self, RIBFIRST, B00000001);
		shiftOut(self, RIBFIRST, B01111111);
		digitalWrite(self->pinLatch, HIGH);

		while (value) {
			digitalWrite(self->pinLatch, LOW);
			shiftOut(self, RIBFIRST, common);
			shiftOut(self, RIBFIRST, led7seg_segments(value % 10));
			digitalWrite(self->pinLatch, HIGH);

			common = common >> 1;
			value /= 10;
		}

	}

// turn off all led
	digitalWrite(self->pinLatch, LOW);
	shiftOut(self, RIBFIRST, B00000000);
	shiftOut(self, RIBFIRST, B11111111);
	digitalWrite(self->pinLatch, HIGH);
}

/*
 #define setBit(x) (PORTD |= (1<<(x)))
 #define clearBit(x) (PORTD &= ~(1<<(x)))

 static void shiftOut(uint8_t bitOrder, uint8_t val){
 uint8_t i;

 for (i = 0; i < 8; i++)  {
 if (bitOrder == LSBFIRST)
 ((val & (1 << i))!=0) ? setBit(Data) : clearBit(Data);
 else
 ((val & (1 << (7 - i)))!=0) ? setBit(Data) : clearBit(Data);

 setBit(Clk);
 clearBit(Clk);
 }
 }
 */
