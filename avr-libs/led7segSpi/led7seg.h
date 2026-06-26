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

#define DEFAULT 0
#define CHOSE_MODE 1
#define TEMP_MATCH 2
#define SLEEP_MODE 3

void sevenSegInit();
void sevenSegDisplay(uint16_t value, uint8_t mode);
void sevenSegIsrDisplay(uint16_t value, uint8_t mode);


#endif /* LED7SEG_H_ */
