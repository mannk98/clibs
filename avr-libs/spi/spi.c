/*
 * spi.c
 *
 *  Created on: Jul 25, 2022
 *      Author: mannk
 */

#include "spi.h"

ringBuff tranferBuff;
ringBuff reciverBuff;

//call when spi_transfer complete
ISR(SPI_STC_vect) {
	uint8_t byte;

	// buffer have elements
	if (!ringBuffEmpty(&tranferBuff)) {
		byte = ringBuff_get(&tranferBuff);
		SPI_DATA = byte;
	}

}

void spiInitMaster(uint32_t clockDiv, uint8_t bitOrder, uint8_t mode) {
	ringBuff_init(&tranferBuff, BUFFER_SIZE);

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
		// master
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
	// SPIF is 0 when in transfer, 1 when transfer complete
	while (!(SPI_STATUS & (1 << SPIF))) {
		;
	}
}

void spiTradeByteISR(uint8_t byte) {
	SPI_CONTROL |= (1 << SPIE);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// if in transfer process
		if (!(SPI_STATUS & (1 << SPIF))) {
			ringBuff_put(&tranferBuff, byte);
		} else
			SPI_DATA = byte;						// start spi_transfer
	}
}

