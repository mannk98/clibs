/*
 * led7seg.c
 *
 *  Created on: Jul 25, 2022
 *      Author: mannk
 */

#include "led7seg.h"
#include "led7seg_font.h"

void sevenSegInit() {
	spiInit(SPI_CLOCK_DIV2, MSBFIRST, MASTERNOISR);
}

// 321	3 -> 2 -> 1, right -> B00001000 <- left	// this funtion and old shiftout(use digitalWrite) take 2.5ms for 4digit display
void sevenSegDisplay(uint16_t value, uint8_t mode, void (*func_delay)(uint16_t)) {
	uint8_t common = B00001000;

	switch (mode) {
	case SLEEP_MODE:
		while (common) {
			SLAVE_SELECT;
			spiTradeByte(common);
			spiTradeByte(B10111111);
			SLAVE_DESELECT;
			common >>= 1;
			func_delay(5);
		}
		break;

	case DEFAULT:
		if (value == 0) {
			SLAVE_SELECT;
			spiTradeByte(common);
			spiTradeByte(led7seg_segments(value));
			SLAVE_DESELECT;
			func_delay(5);
		} else {
			while (value) {
				SLAVE_SELECT;
				spiTradeByte(common);
				spiTradeByte(led7seg_segments(value % 10));
				SLAVE_DESELECT;
				common = common >> 1;
				value /= 10;
				func_delay(5);
				//SLAVE_DESELECT;
			}
		}
		break;

	case CHOSE_MODE:
		SLAVE_SELECT;
		spiTradeByte(B00000001);
		spiTradeByte(B11110111);
		SLAVE_DESELECT;
		func_delay(5);
		if (value == 0) {
			SLAVE_SELECT;
			spiTradeByte(common);
			spiTradeByte(led7seg_segments(value));
			SLAVE_DESELECT;
			func_delay(5);
		} else {
			while (value) {
				SLAVE_SELECT;
				spiTradeByte(common);
				spiTradeByte(led7seg_segments(value % 10));
				SLAVE_DESELECT;
				common = common >> 1;
				value /= 10;
				func_delay(5);
			}
		}
		break;

	case TEMP_MATCH:
		SLAVE_SELECT;
		spiTradeByte(B00000001);
		spiTradeByte(B01111111);
		SLAVE_DESELECT;
		func_delay(5);
		if (value == 0) {
			SLAVE_SELECT;
			spiTradeByte(common);
			spiTradeByte(led7seg_segments(value));
			SLAVE_DESELECT;
			func_delay(5);
		} else {
			while (value) {
				SLAVE_SELECT;
				spiTradeByte(common);
				spiTradeByte(led7seg_segments(value % 10));
				SLAVE_DESELECT;
				common = common >> 1;
				value /= 10;
				func_delay(5);
			}
		}
		break;
	}
// turn off all led
	SLAVE_SELECT;
	spiTradeByte(B00000000);
	spiTradeByte(B11111111);

	SLAVE_DESELECT;
}

// display dot(.) in the rightmost
void sevenSegTurnOff() {
	SLAVE_SELECT;
	spiTradeByte(B00000000);
	spiTradeByte(B11111111);
	SLAVE_DESELECT;
	SLAVE_SELECT;
	spiTradeByte(B00001000);
	spiTradeByte(B01111111);
	SLAVE_DESELECT;
}
