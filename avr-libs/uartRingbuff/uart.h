/*
 * simpleUART.h
 *
 *  Created on: Mar 23, 2022
 *      Author: mannk
 */
// acssi II
#ifndef SIMPLEUART_H_
#define SIMPLEUART_H_

#include <avr/interrupt.h>
//#include <util/delay.h>
#include <util/atomic.h>	// not call Interrupt (get(ringbuff)) when call put(ringbuff);
#include "ringbuff.h"

#define endl "\n\r"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BUFFERSIZE 128

// this for UBRROH and UBRROL, this register count down, each time reach 0 -> 1 clk
// normal mode
#define UART_BAUD_SELECT(baudRate,xtalCpu) (((xtalCpu)/(16UL)/(baudRate))-1UL)
//#define UART_BAUD_SELECT(baudRate,xtalCpu) (((xtalCpu)+8UL*(baudRate))/(16UL*(baudRate))-1UL)
// x2 mode
#define UART_BAUD_SELECT_DOUBLE_SPEED(baudRate,xtalCpu) ((((xtalCpu)+4UL*(baudRate))/(8UL*(baudRate))-1)|0x8000)

#define UART0_STATUS   UCSR0A
#define UART0_CONTROL  UCSR0B
#define UART0_DATA     UDR0
/* uart data register interrupt enable -> when this bit is set, call USART_UDRE_vect as long as UDRE0 is set.
 * (default write UDRE0 0, auto set 1 when buffer tx have data)
 */
#define UART0_UDRIE    UDRIE0

/*
 ** high byte error return code of uart_getc()
 */
#define UART_FRAME_ERROR      0x0800              /**< Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0400              /**< Overrun condition by UART   */
#define UART_BUFFER_OVERFLOW  0x0200              /**< receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /**< no receive data available   */

#ifndef BAUDRATE
#define BAUDRATE 9600
#endif

uint8_t uart_init(uint32_t baudrate);
void uart_putChar(uint8_t data);
void uart_putString(uint8_t *string);
void uart_putInteger(uint32_t number);

//uint8_t recieveByte();
//uint8_t recieceString(uint8_t str[], uint8_t maxlen);

#endif /* SIMPLEUART_H_ */

