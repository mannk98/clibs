/*
 * wiring_digital.c
 *
 *  Created on: Apr 7, 2022
 *      Author: mannk
 */

#define ARDUINO_MAIN				// define va vi tri cua ARDUINO_MAIN??

#include "wiring_digital.h"
#include <avr/interrupt.h>

void pinMode(uint8_t pin, uint8_t mode){
	uint8_t bit, port;
	bit = digitalPinToBitMask(pin);	// return 0 -> 7
	port = digitalPinToPort(pin); // return 2or3or4 <=> PB,PC,PD.
	volatile uint8_t *reg, *out;

	if(port == NOT_A_PORT) return;

	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if(mode == INPUT){
		uint8_t oldSREG = SREG;	// save status of status register
		cli();					// disable interrupt
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	}
	else if (mode == INPUT_PULLUP){
		uint8_t oldSREG = SREG;
		cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	}
	else if(mode == OUTPUT){
		uint8_t oldSREG = SREG;
		cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

void digitalWrite(uint8_t pin, uint8_t val){
	uint8_t bit, port;
	bit = digitalPinToBitMask(pin);
	port = digitalPinToPort(pin);
	volatile uint8_t *out;
	out = portOutputRegister(port);

	if(port == NOT_A_PORT) return;

	uint8_t oldSREG = SREG;
	cli();
	if(val == HIGH){
		*out |= bit;
	}
	else if(val == LOW){
		*out &= ~bit;
	}

	SREG = oldSREG;
}

uint8_t digitalRead(uint8_t pin){
	uint8_t bit, port;
	bit = digitalPinToBitMask(pin);
	port = digitalPinToPort(pin);

	volatile uint8_t *in;
	in = portInputRegister(port);

	if(port == NOT_A_PORT) return LOW;
	if(*in & bit) return HIGH;
	return LOW;
}

void pinTongle(uint8_t pin) {
	if (digitalRead(pin) == HIGH) {
		digitalWrite(pin, LOW);
	} else
		digitalWrite(pin, HIGH);
}



