/*
 * pwmTimer1.c
 *
 *  Created on: Apr 20, 2022
 *      Author: mannk
 */

#include "Timer.h"

// use ICR1 register as TOP if use fix top value, and can use OCR1A to generate PWM output for OC1A
// use OCR1A register as TOP: can change top value anytime (it have double buffer). --> only generate PWM output for OC1B

// pwm on 0C1B - PB2 - pin 10 arduino
// freq < 64kHz
// OCR1A = 2000;	for 1kHZ
// not revert output ( oc1b revert: set at cmp match, clear at clk top to bottom(0).)
void initPwm_oc1b(uint16_t freq) {	// set TIMER as PWM Mode, but not start it;
	cli();
	TCNT1 = 0;
	OCR1A = 16000000 / (8 * freq);

	// top = OCR1A (WGM13:0 = 15)
	TCCR1A |= (1 << WGM11) | (1 << WGM10);
	TCCR1B |= (1 << WGM13) | (1 << WGM12);

	// prescale = 8
	TCCR1B |= (1 << CS11);

	// revert output mode oc1b
	TCCR1A |= (1 << COM1B1) | (1 << COM1B0);

	// turn of PWM
	OCR1B = 0;

	// set OC1B as output --> enable PWM
	DDRB |= (1 << OC1B);

	// ??? if use
	// enable OVF ISR, to count number of time OVF(vd: 100 time OVF ->change ratio(OCR1B)) 
	TIMSK1 |= (1 << TOIE1);

	sei();
	//OCR1A = 2000;

}

void updatePwm_oc1b(uint16_t ratio) {// ratio(high) = OCR1A/OCR1B	1<ratio<1000
	uint32_t temp = OCR1A;
	temp = (temp * ratio) / 1000;
	OCR1B = temp;
}

// revert mode
void initPwm_oc1a(uint16_t freq) {
	cli();
	TCNT1 = 0;

	ICR1 = 16000000 / (8 * freq);

	// top = ICR1 (WGM13:0 = 14)
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM13) | (1 << WGM12);

	// prescale = 8
	TCCR1B |= (1 << CS11);

	// revert output mode oc1b
	TCCR1A |= (1 << COM1A1) | (1 << COM1A0);

	// turn of PWM
	OCR1A = 0;

	// set OC1B as output --> enable PWM
	DDRB |= (1 << OC1A);

	sei();
}

void updatePwm_oc1a(uint16_t ratio) {// ratio(high) = OCR1A/OCR1B	1<ratio<1000
	uint32_t temp = ICR1;
	temp = (temp * ratio) / 1000;
	OCR1A = temp;
}

void timer1_stop() {
	TCCR1B &= ~(1 << CS11);			// set prescaler to none, disabling timer
}

void timer1_resume() {
	TCCR1B |= (1 << CS11);			// set prescaler to none, disabling timer
}

/*
 // CTC timer2
 void initCTCTimer2(int f){
 cli();
 TCNT2   = 0;
 TCCR2A |= (1<<WGM21);
 TCCR2B |= (1<<CS22) |(1<<CS21) | (1<<CS20);
 OCR2A = f;		// =48 for 325 refresh rate
 TIMSK2 |= (1<<OCIE2A);
 sei();
 }
 */
