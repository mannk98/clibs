/*
 * pwmTimer1.h
 *
 *  Created on: Apr 20, 2022
 *      Author: mannk
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <avr/interrupt.h>

#define OC1B PB2
#define OC1A PB1

void initPwm_oc1b(uint16_t freq);
void updatePwm_oc1b(uint16_t ratio);
void initPwm_oc1a(uint16_t freq);
void updatePwm_oc1a(uint16_t ratio);

void timer1_stop();
void timer1_resume();

//void initCTCTimer2(int f);

#endif /* TIMER_H_ */

