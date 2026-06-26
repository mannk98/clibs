/*
 * led7seg.h
 *
 *  Created on: Jul 25, 2022
 *      Author: mannk
 */

#ifndef LED7SEG_H_
#define LED7SEG_H_

#include "spi.h"
#include "binary.h"

#define DEFAULT 1
#define CHOSE_MODE 2
#define TEMP_MATCH 3
#define SLEEP_MODE 4

void sevenSegInit();
void sevenSegDisplay(uint16_t value, uint8_t mode, void (*func_delay)(uint16_t));
void sevenSegTurnOff();

#endif /* LED7SEG_H_ */
