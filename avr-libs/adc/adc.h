/*
 * singerADC.h
 *
 *  Created on: Mar 28, 2022
 *      Author: mannk
 */

#ifndef SIMPLEADC_H_
#define SIMPLEADC_H_

#include <avr/interrupt.h>			// already include avr/io.h
//#include <avr/sleep.h>			// for sleep interrupt, ISR weakup

#define VREF1_1 192					// 1.1V
#define AVCC 64

#define PRESCALE_2 1
#define PRESCALE_4 2
#define PRESCALE_8 3
#define PRESCALE_16 4
#define PRESCALE_32 5
#define PRESCALE_64 6
#define PRESCALE_128 7				// mostly use

/*
 #ifndef A7
 enum {
 A0, A1, A2, A3, A4, A5, A6, A7
 };
 #endif
 */

#define EIGHT 1
#define TEN 0

void adc_stop();
void adc_init(uint8_t vref, uint8_t bitmode, uint8_t prescale);

uint16_t adc_singeRead(uint8_t pin);

// start ISR conversion, adc_isrReturn return average of last 8 times read_adc
void adc_isrStartRead(uint8_t pin);
uint16_t adc_isrReturn();

//uint16_t readADCSleepMode(uint8_t pin);

#endif /* SIMPLEADC_H_ */
