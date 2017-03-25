/*
 * ringBuffer.c
 *
 *  Author: thagemeier
 */ 

#include "ringBuffer.h"
#include <avr/interrupt.h>

// Größe ca: 344 Bytes

// ACHTUNG: DER PUFFER DARF NICHT MIT MEHR ALS 16 KB GRÖßE BETRIEBEN WERDEN!!

RB_BUFFER_t * rb_createBuffer(void * data, unsigned short int buflen) {
	RB_BUFFER_t * buf = (RB_BUFFER_t*)data;
	if (buflen > (sizeof(RB_BUFFER_t) + 2)) {
		buf->buffer = ((unsigned char *)data) + sizeof(RB_BUFFER_t);
		buf->readPosition = 0;
		buf->writePosition = 0;
		buf->size = 0;
		buf->maxSize = buflen - sizeof(RB_BUFFER_t);
		return buf;
	} else {
		return (RB_BUFFER_t*)0;
	}
}

unsigned char rb_peek(RB_BUFFER_t * buf, unsigned short int position) {
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		return buf->buffer[position];
	}
	return 0;
}

void rb_delete(RB_BUFFER_t * buf, unsigned short int howMany) {
	if (howMany && (howMany < buf->size)) {
		buf->readPosition += howMany;
		buf->size -= howMany;
		if (!(buf->readPosition < buf->maxSize))
			buf->readPosition -= buf->maxSize;
	} else { // Alles löschen
		buf->readPosition = buf->writePosition;
		buf->size = 0;
	}
}

unsigned char rb_put(RB_BUFFER_t * buf, unsigned char whatChar) {
	if (buf->size < buf->maxSize) {
		buf->buffer[buf->writePosition++] = whatChar;
		buf->size++;
		if (!(buf->writePosition < buf->maxSize))
			buf->writePosition = 0; // Das geht nur, da wir jedes Mal höchstens ein Byte hinzufügen
		return 1;
	} else {
		return 0;
	}
}

unsigned char rb_get(RB_BUFFER_t * buf) {
	unsigned char data = rb_peek(buf, 0);
	rb_delete(buf, 1);
	return data;
}

unsigned short int rb_getCount(RB_BUFFER_t * buf) {
	return buf->size;
}

unsigned char rb_peek_sync(RB_BUFFER_t * buf, unsigned short int position) {
	unsigned char data;
	cli();
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		data = buf->buffer[position];
		sei();
		return data;
	}
	sei();
	return 0;
}

void rb_delete_sync(RB_BUFFER_t * buf, unsigned short int howMany) {
	cli();
	if (howMany && (howMany < buf->size)) {
		buf->readPosition += howMany;
		buf->size -= howMany;
		if (!(buf->readPosition < buf->maxSize))
			buf->readPosition -= buf->maxSize;
	} else { // Alles löschen
		buf->readPosition = 0;
		buf->writePosition = 0;
		buf->size = 0;
	}
	sei();
}

unsigned char rb_put_sync(RB_BUFFER_t * buf, unsigned char whatChar) {
	cli();
	if (buf->size < buf->maxSize) {
		buf->buffer[buf->writePosition++] = whatChar;
		buf->size++;
		if (!(buf->writePosition < buf->maxSize))
			buf->writePosition = 0; // Das geht nur, da wir jedes Mal höchstens ein Byte hinzufügen
		sei();
		return 1;
	} else {
		sei();
		return 0;
	}
}

unsigned char rb_get_sync(RB_BUFFER_t * buf) {
	unsigned char data = rb_peek_sync(buf, 0);
	rb_delete_sync(buf, 1);
	return data;
}

unsigned short int rb_getCount_sync(RB_BUFFER_t * buf) {
	unsigned short int count;
	cli();
	count = buf->size;
	sei();
	return count;
}
