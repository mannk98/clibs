/*
 * ringbuff.h
 *
 *  Created on: Jul 15, 2022
 *      Author: mannk
 */

//@breif Ring Buffer use for char (ASCII)
#ifndef RINGBUFF_H_
#define RINGBUFF_H_

#include <stdlib.h>

//#define NULL ((void*)0)

#define uint8_t char

enum {
	RB_SUCCESS = 0, RB_FULL = -1, RB_EMPTY = -2, ERROR = -3, RB_BUSY = -4
};

typedef struct {
	uint8_t *buffer;
	uint8_t *endBuffer;
	uint8_t capacity;
	uint8_t isBusy;
	uint8_t numberElement;
	uint8_t sizeOfElement; // size of element default = 1 (char), maybe update it uint32_t, float, pointer, or struct.
	uint8_t *head;
	uint8_t *tail;
} ringBuff;

int ringBuff_init(ringBuff *self, uint8_t cap);
int ringBuff_free(ringBuff *self);
int ringBuff_reset(ringBuff *self);

int ringBuff_put(ringBuff *self, uint8_t element);
int ringBuff_putString(ringBuff *self, const uint8_t *string);
uint8_t ringBuff_get(ringBuff *self);
int ringBuff_getBytes(ringBuff *self, uint8_t *dataOut, uint8_t lenght);

int ringBuffEmpty(ringBuff *self);
int ringBuffFull(ringBuff *self);

#endif /* RINGBUFF_H_ */
