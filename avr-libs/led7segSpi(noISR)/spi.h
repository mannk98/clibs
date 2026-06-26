/*
 * spi.h
 *
 *  Created on: Jul 25, 2022
 *      Author: mannk
 */

#include <avr/io.h>
#include <util/atomic.h>

#ifndef SPI_H_
#define SPI_H_

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif

// register define
#define SPI_STATUS SPSR
#define SPI_CONTROL SPCR
#define SPI_DATA SPDR

// pin define
#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define MISO PB4
#define MOSI PB3

#define SS PB2

#define SCK PB5

// clock
#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

// mode
#define MASTERNOISR 0x01
#define MASTERISR 0x02
#define SLAVENOISR 0x03
#define SLAVEISR 0x04

#define BUFFER_SIZE 64

#define SLAVE_SELECT SPI_PORT &= ~(1<<SS)
#define SLAVE_DESELECT SPI_PORT |= (1<<SS)

void spiInit(uint32_t clockDiv, uint8_t bitOrder, uint8_t mode);
void spiTradeByte(uint8_t byte);

#endif /* SPI_H_ */
