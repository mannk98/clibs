/*
 * ringbuff.c
 *
 *  Created on: Jul 15, 2022
 *      Author: mannk
 */

#include "ringbuff.h"

int ringBuff_init(ringBuff *self, uint8_t cap) {

	self->capacity = cap;
	self->buffer = (uint8_t*) malloc(sizeof(uint8_t) * self->capacity);
	if (self->buffer == NULL) {
		return ERROR;
	}
	self->endBuffer = self->buffer + cap - 1;
	self->numberElement = 0;
	self->sizeOfElement = sizeof(uint8_t);
	self->head = self->buffer;
	self->tail = self->buffer;

	self->isBusy = 0;
	return RB_SUCCESS;
}

int ringBuff_free(ringBuff *self) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;
	// assume that ringBuff has no element
	self->numberElement = 0;
	self->tail = self->head;
	free(self->buffer);

	//self->isBusy = 0;
	return RB_SUCCESS;
}

int ringBuff_reset(ringBuff *self) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;

	self->tail = self->head = self->buffer;
	self->numberElement = 0;

	self->isBusy = 0;
	return RB_SUCCESS;
}

// take ~10 micro second on 328p 16MHz
// head: oldest, tail: latest
int ringBuff_put(ringBuff *self, uint8_t element) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;

	if (self->numberElement < self->capacity) {
		self->tail++;
		if (self->tail > self->endBuffer) {				// meet end off buffer
			self->tail = self->buffer;
		}
		*(self->tail) = element;
		self->numberElement++;
	} else if (ringBuffFull(self)) {					// ringbuff-full
		while (ringBuffFull(self))
			// loop until buffer has space. (ringBuff_get() in ISR)
			;
		self->tail++;
		if (self->tail > self->endBuffer) {
			self->tail = self->buffer;
		}
		*(self->tail) = element;
		self->numberElement++;
	}

	self->isBusy = 0;
	return RB_SUCCESS;
}

// if ringBuffer override -> return 1
int ringBuff_putString(ringBuff *self, const uint8_t *string) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;

	while (*string != NULL) {
		ringBuff_put(self, *string);
		string++;
	}

	self->isBusy = 0;
	return RB_SUCCESS;
}

// just read data, change head, tail pointer. Not delete data.
uint8_t ringBuff_get(ringBuff *self) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;

	uint8_t val;

	if (ringBuffEmpty(self)) {
		return RB_EMPTY;
	}
	self->head++;
	if (self->head > self->endBuffer) {
		self->head = self->buffer;
	}
	val = *(self->head);
	self->numberElement--;

	self->isBusy = 0;
	return val;
}

int ringBuff_getBytes(ringBuff *self, uint8_t *dataOut, uint8_t len) {
	if (self->isBusy == 1)
		return RB_BUSY;
	self->isBusy = 1;

	uint8_t count = 1;
	uint8_t *temp;

	temp = dataOut;
	while (!ringBuffEmpty(self) && count <= len) {
		*temp = ringBuff_get(self);
		temp++;
		count++;
	}

	self->isBusy = 0;
	if (count == len - 1)
		return RB_SUCCESS;
	return RB_EMPTY;
}

int ringBuffEmpty(ringBuff *self) {
	if (self->numberElement == 0 && self->tail == self->head) {
		return 1;
	}
	return 0;
}

int ringBuffFull(ringBuff *self) {
	if (self->numberElement == self->capacity) {
		return 1;
	}
	return 0;
}
