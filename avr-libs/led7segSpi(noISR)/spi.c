/*
 * spi.c
 *
 *  Created on: Jul 25, 2022
 *      Author: mannk
 */

#include "spi.h"

void spiInit(uint32_t clockDiv, uint8_t bitOrder, uint8_t mode) {
	if (clockDiv <= SPI_CLOCK_DIV128) {
		SPI_CONTROL |= clockDiv;
	} else {
		SPI_CONTROL |= clockDiv;
		SPI_CONTROL &= ~(1 << CPHA);
		SPI_STATUS |= (1 << SPI2X);
	}

	//CPHA & CPOL set 0 as default

	if (bitOrder == LSBFIRST)
		SPI_CONTROL |= (1 << DORD);
	else
		SPI_CONTROL &= ~(1 << DORD);

	if (mode == MASTERNOISR || mode == MASTERISR) {
		SPI_CONTROL |= (1 << MSTR);

		// set as out put
		SPI_DDR |= (1 << MOSI) | (1 << SCK) | (1 << SS);
		// pull up
		SPI_PORT |= (1 << MOSI) | (1 << SS);
	}

	SPI_CONTROL |= (1 << SPE);
}

void spiTradeByte(uint8_t byte) {
	SPI_DATA = byte;
	while (!(SPI_STATUS & (1 << SPIF))) {
		;
	}
}

