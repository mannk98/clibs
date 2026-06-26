/*
 * simpleUART.h
 *
 *  Created on: Mar 23, 2022
 *      Author: mannk
 */

#ifndef BAUD
#define BAUD 9600
#endif

#ifndef SIMPLEUART_H_
#define SIMPLEUART_H_

#include <avr/io.h>
#include <util/setbaud.h>

#define endl "\n\r"

void initUART();

void transmitByte(uint8_t data);			// acssi II
void transmitString(uint8_t str[]);
void transmitBinaryByte(uint8_t data);

void transmitDecimaByte(uint8_t byte);
void transmitDecimaWord(uint16_t _word);
void transmitDecimaDWord(uint32_t _dWord);

uint8_t recieveByte();
uint8_t recieceString(uint8_t str[], uint8_t maxlen);

#endif /* SIMPLEUART_H_ */

