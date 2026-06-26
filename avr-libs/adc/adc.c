/*
 * SingerADC.c
 *
 *  Created on: Mar 28, 2022
 *      Author: mannk
 */
// read 10bit adc
#include "adc.h"

#define ENABLE_ADC_INT

volatile uint16_t adc_buffer[7]; 		// 8 elements
volatile uint8_t number_time_adc_isr_read;
volatile uint16_t analogVal;
volatile uint16_t analogValResult;

void adc_stop() {
	ADCSRA &= ~(1 << ADEN);
}

void adc_init(uint8_t vref, uint8_t bitmode, uint8_t prescale) {
	ADMUX |= vref;
	ADCSRA |= prescale;
	if (bitmode) {
		ADMUX |= (1 << ADLAR);
	}
	ADCSRA |= (1 << ADEN);
}

uint16_t adc_singeRead(uint8_t pin) {
	uint16_t result = 0;

	//  0 -> 6 <--> pinADC (0 ->6)
	ADMUX = (0xf0 & ADMUX) | pin;

	for (uint8_t i = 0; i < 8; i++) {
		ADCSRA |= (1 << ADSC);

		while (ADCSRA & (1 << ADSC))
			// loop until ADSC cleared -> done conversion
			;

		if ((1 << ADLAR) & ADMUX) {			//  8bits mode
			result += ADCH;
		} else
			result += ADC;					// 10bits mode
	}
	return (result >> 3);
}

void adc_isrStartRead(uint8_t pin) {
	ADCSRA |= (1 << ADATE);
	ADCSRA |= (1 << ADIE);
	ADCSRA |= (1 << ADSC);

	ADMUX = (0xf0 & ADMUX) | pin;
}

uint16_t adc_isrReturn() {
	return analogValResult;
}

// trigger by ADIF flag is set(when conversion completes and Data register are updated)
// after ISR executed, ADIF and ADSC are cleared.
// each time read take 13clk adc.

ISR(ADC_vect) {
	if (number_time_adc_isr_read >= 8) {
		for (int i = 0; i < 8; i++) {
			analogVal += adc_buffer[i];
		}
		analogVal /= 8;

		analogValResult = analogVal;
		analogVal = 0;
		number_time_adc_isr_read = 0;
		ADCSRA &= ~(1 << ADIE);			// stop conversion when buffer is full
	} else {
		if ((1 << ADLAR) & ADMUX) {								//  8bits mode
			adc_buffer[number_time_adc_isr_read] = ADCH;
		} else
			adc_buffer[number_time_adc_isr_read] = ADC;			// 10bits mode

		number_time_adc_isr_read++;

		ADCSRA |= (1 << ADSC);							// start next conversion
	}

	//ADCSRA &= ~(1 << ADIE);
}

