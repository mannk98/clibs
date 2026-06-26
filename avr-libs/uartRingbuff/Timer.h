/*
 * pwmTimer1.h
 *
 *  Created on: Apr 20, 2022
 *      Author: mannk
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <avr/interrupt.h>


void initPwm(uint16_t freq);
void updatePwm(uint16_t ratio);

void initCTCTimer2(int f);


#endif /* TIMER_H_ */

