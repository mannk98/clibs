/*
 * main.c
 *
 *  Created on: Aug 8, 2022
 *      Author: mannk
 */

#include "sonRTOS.h"
#include "arduinoPinMapping/wiring_digital.h"
#include "uart/uart.h"

#define LED 12

void uartTask();
void blink1();

int main() {
	pinMode(LED, OUTPUT);
	digitalWrite(LED, HIGH);

	uart_init(9600);
	port_InitOSTimer2();

	//priority: task0 -> highest, taskn: lowest
	port_OS_TASK_INIT((u16) blink1, 128, task1);
	port_OS_TASK_INIT((u16) uartTask, 256, task0);

	osSetRTR(task0);
	osSetRTR(task1);
	asm("sei");

	__schedule();
}

void blink1() {
	while (1) {
		pinTongle(LED);
		osWaitMs(200);
	}
}

void uartTask() {
	while (1) {
		uart_putString("test uart\n\r");
		osWaitMs(1000);
	}
}

