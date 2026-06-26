/*
 * wallClock.h
 *
 *  Created on: Apr 20, 2022
 *      Author: mannk
 */

#ifndef WALLCLOCK_H_
#define WALLCLOCK_H_

#include <avr/interrupt.h>

void initWallClock();
uint32_t millis();
uint32_t micros();

#endif /* WALLCLOCK_H_ */
