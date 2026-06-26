/*
 * test.c
 *
 *  Created on: Jul 17, 2022
 *      Author: mannk
 */

#include "uart.h"
#include <util/delay.h>
#include "wallClock.h"

#define BAUDRATE 9600

int main() {
	initWallClock();
	uart_init(BAUDRATE);
	sei();
	uint32_t preTime, period;
//	uint32_t x = 0;
	float m,n ;
	m = 3.144;
	n = 111.124;
//	float result;
//	uint32_t result1;
//	uint32_t g,h;
//	g =3140; h = 111123;

	char a = 'a';
	uart_putChar(a++);
	uart_putChar(a++);
	uart_putChar(a++);
	//uart_putChar(a++);
	//uart_putChar(a++);
	uart_putChar(a++);
	uart_putChar(a++);
	//ring_buff_init();
	//UART0_CONTROL |= (1 << UART0_UDRIE);
	uart_putString("\n\r");
	//_delay_ms(1000);
	uart_putString("String stored in SRA");
	//_delay_ms(1000);
	uart_putString("String stored in SRAM\r\n");

	preTime = micros();
//	result = m + n;
	//result1 = g + h;
	period = micros() - preTime;
	uart_putInteger(period);
	uart_putString("\r\n");


	while(1){
		//uart_putString("dump prog now not dump\n\r");
		//uart_putInteger(x++);
		//uart_putString("\n\r");
		//uart_puts("dump prrog\n\r");
		//uart_puts("dump prrog\n\r");
		//uart_puts("dump prrog\n\r");

		//ring_buff_init();
		//UART0_CONTROL |= (1 << UART0_UDRIE);
		_delay_ms(100);
		uart_putInteger(a);
		uart_putString("\r\n");

	}
	//uart_put_test();

	/*while(1){
		//uart_puts("String stored in SRAM\n");
		uart_putc('S');
		uart_putc('t');
		uart_putc('r');
		uart_putc('i');
		uart_putc('n');
		uart_putc('g');
		uart_putc(' ');
		uart_putc('s');
		uart_putc('t');


		uart_putc('\n');
		uart_putc('\r');
		_delay_ms(1000);
	}*/

	return 0;
}


