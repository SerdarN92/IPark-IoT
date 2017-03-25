#include "common.h"
#include <avr/io.h>
#include <util/delay.h> 	// Delay-Funktionen
#include <avr/interrupt.h>
#include "systimer.h"




unsigned int toWait = 0;

void busy_wait(unsigned short milliseconds) {
	while (milliseconds > 16) {
		milliseconds-=16;
		_delay_ms(16);
	}
	if (milliseconds & 8)
		_delay_ms(8);
	if (milliseconds & 4)
		_delay_ms(4);
	if (milliseconds & 2)
		_delay_ms(2);
	if (milliseconds & 1)
		_delay_ms(1);
}

void wait(unsigned short milliseconds) {
	// Diese Funktion stellt sicher, das _delay_ms nur maximal 16ms warten muss (sonst läuft dort der Puffer über..)
	// [bei 7,3728MHz eigentlich 35ms aber wir wollen ja irgendwann auf 16MHz hoch mit dem Prozitakt]
	if (systimer_isTimerRunning()) {
		start_wait(milliseconds);
		while (do_wait()) {
			
		}
	} else {
		while (milliseconds > 16) {
			milliseconds-=16;
			_delay_ms(16);
		}
		if (milliseconds & 8)
			_delay_ms(8);
		if (milliseconds & 4)
			_delay_ms(4);
		if (milliseconds & 2)
			_delay_ms(2);
		if (milliseconds & 1)
			_delay_ms(1);
	}
}

void start_wait(unsigned int howlong) {
	check_timers();
	toWait = howlong;
}

unsigned int do_wait() {
	unsigned char elapsed = 0;
	elapsed = (unsigned char)check_timers();
	if (elapsed > toWait) {
		toWait = 0;
	} else if (elapsed) {
		toWait-= elapsed;
	}
	return toWait;
}

/*
unsigned long int swapBytes32(unsigned long int bt) {
	unsigned long int retval;
	unsigned char ptr1 = (unsigned char*)bt;
	unsigned char ptr2 = (unsigned char*)retval;
	ptr2[0] = ptr1[3];
	ptr2[1] = ptr1[2];
	ptr2[2] = ptr1[1];
	ptr2[3] = ptr1[0];
	return retval;
}
*/

void reverseByteOrder(void * p, unsigned char len) {
	unsigned char i;
	unsigned char * ptr = (unsigned char*)p;
	unsigned char tmp;
	for (i=0;i<((len+1)/2);i++) {
		tmp = ptr[len - i - 1];
		ptr[len - i - 1] = ptr[i];
		ptr[i] = tmp;
	}
}

unsigned long int atoi_long(const unsigned char * p, unsigned char len) {
	unsigned long int value = 0;
	while (len--) {
		if (*p >= '0' && *p <= '9') {
			value *= 10;
			value += ((*p++) - '0');
		} else {
			break;
		}
	}
	return value;
}
