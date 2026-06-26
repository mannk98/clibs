/*
 * simple-UART.c
 *
 *  Created on: Mar 23, 2022
 *      Author: mannk
 */

#include "uart.h"
//#include <stdio.h>	// NULL in stdio.h is pointer


ringBuff self;

/*************************************************************************
 Function: UART Data Register Empty interrupt
 Purpose:  called when the UART is ready to transmit the next byte
 **************************************************************************/

// if baudrate = 9600 --> ~1ms for each ISR (UDREn: USART Data Register Empty set 1)
ISR(USART_UDRE_vect) {
	uint8_t tempData;
	if (ringBuffEmpty(&self)) {
		/* tx buffer empty, disable UDRE interrupt */
		UART0_CONTROL &= ~(1 << UART0_UDRIE);
	} else {

		tempData = ringBuff_get(&self);
		/* start transmission */
		UART0_DATA = tempData;
	}
}

// simple uart using polling - just for fun.
uint8_t uart_init(uint32_t baudrate) {
	uint32_t baudSelect = UART_BAUD_SELECT(baudrate, F_CPU);
	uint8_t err;
	// ringbuff init
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		err = ringBuff_init(&self, BUFFERSIZE);
		if (err == ERROR) {
			return 1;
		}
	}
	// baudrate
	if (baudSelect & 0x8000) {
		UART0_STATUS = (1 << U2X0);  //Enable 2x speed
		baudSelect &= ~0x8000;
	} else
		UART0_STATUS &= ~(1 << U2X0);

	UBRR0H = (uint8_t) (baudSelect >> 8);
	UBRR0L = (uint8_t) baudSelect;

	// set control bit
	UART0_CONTROL |= (1 << RXEN0) | (1 << TXEN0); // |_BV(RXCIE0)|
	UART0_CONTROL &= ~(1 << UDRE0) | ~(1 << UDRIE0);

	/* Set frame format: asynchronous, 8data, no parity, 1stop bit */
	UCSR0C |= (3 << UCSZ00);
	return 0;
}

/* uart0_putc */
void uart_putChar(uint8_t data) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ringBuff_put(&self, data);
		/* enable UDRE interrupt */
		UART0_CONTROL |= (1 << UART0_UDRIE);
	}
}

void uart_putString(uint8_t *string) {
	while (*string != NULL) {
		uart_putChar(*string);
		string++;
	}
}

void uart_putInteger(uint32_t number) {
	if (number == 0) {
		uart_putChar('0');
		return;
	}
	uint8_t numBefore = 0;
	uint8_t num;
	uint32_t div = 100000000;

	while (div > 0) {
		if ((num = (number / div) % 10) > 0 || numBefore) {
			uart_putChar('0' + num);
			numBefore = 1;
		}
		div /= 10;
	}
}

