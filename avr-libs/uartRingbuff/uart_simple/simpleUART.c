/*
 * simple-UART.c
 *
 *  Created on: Mar 23, 2022
 *      Author: mannk
 */

#include "simpleUART.h"

//#include <stdio.h>	// NULL in stdio.h is pointer


// simple uart using polling - just for fun.
void initUART(){
	UBRR0H = UBRRH_VALUE;	// value after define baudrate
	UBRR0L = UBRRL_VALUE;
#if USE_2X
	UCSR0A |= (1 << U2X0);
#else
	UCSR0A &= ~(1 << U2X0);
#endif
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0);	// enable tx and rx
	UCSR0C |= (3 << UCSZ00);	// set 8 bit data
}


// ///////////////// TRANSMIT
void transmitByte(uint8_t data){
	while(!(UCSR0A &= (1<<UDRE0))){
		;
	}
	UDR0 = data;
}

void transmitString(uint8_t str[]){
	while(*str){
		transmitByte(*str++);
	}
}

void transmitBinaryByte(uint8_t data){
	int bitmask = 7;
	while(bitmask >= 0){
		if(data & (1<<bitmask))
			transmitByte('1');
		else transmitByte('0');
		bitmask--;
	}
}

void transmitDecimaByte(uint8_t byte) {		// uint8
	if(byte == 0){
		transmitByte('0');
		return;
	}
              /* Converts a byte to a string of decimal text, sends it */
  transmitByte('0' + (byte / 100));                        /* Hundreds */
  transmitByte('0' + ((byte / 10) % 10));                      /* Tens */
  transmitByte('0' + (byte % 10));                             /* Ones */
}

void transmitDecimaWord(uint16_t _word){		// uint16
/*	  transmitByte('0' + (_word / 10000));
	  transmitByte('0' + ((_word / 1000) % 10));
	  transmitByte('0' + (_word / 100) %10);                        // Hundreds
	  transmitByte('0' + ((_word / 10) % 10));                      // Tens
	  transmitByte('0' + (_word % 10));                             // Ones
	  */
	if(_word == 0){
		transmitByte('0');
		return;
	}
	uint8_t numBefore = 0;
	uint8_t num;
	if(_word > 10000){
		transmitByte('0' + (_word / 10000));
		numBefore = 1;
	}
	uint16_t div = 1000;

	while(div > 0){
		if((num =(_word / div)%10) > 0 || numBefore){
			transmitByte('0'+num);
			numBefore = 1;
		}
		div /= 10;
	}
}

void transmitDecimaDWord(uint32_t _dWord){
	if(_dWord == 0){
		transmitByte('0');
		return;
	}
	uint8_t numBefore = 0;
	uint8_t num;
	if(_dWord > 1000000000){
		transmitByte('0' + (_dWord / 1000000000));
		numBefore = 1;
	}
	uint32_t div = 100000000;

	while(div > 0){
		if((num =(_dWord / div)%10) > 0 || numBefore){
			transmitByte('0'+num);
			numBefore = 1;
		}
		div /= 10;
	}
}


// ///////////////////////// RECIEVE
uint8_t recieveByte(){
	while(!(UCSR0A &= (1<<RXC0))){
		;
	}
	return UDR0;
}

uint8_t recieceString(uint8_t str[], uint8_t maxlen){	// reciece any thing is string(str, binary bits, ...).	// arduino monitor send endline with \r\n
	uint8_t reciByte;
	uint8_t i = 0;
	while(i < (maxlen-1)){
		reciByte = recieveByte();
		if(reciByte == '\r'){
			break;
		}
		else str[i] = reciByte;
		i++;
	}
	str[i] = 0;	// NULL
	return (i-1); // ignore NULL
}







