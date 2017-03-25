#include "servo.h"
#include "console.h"


// TC0_t

void servo_init(TC0_t * tc) {
	// Servo will ca. alle 20ms ein Positionsupdate.
	// Bei einem Takt von 32 MHz sind 20ms = 640.000 Takte. Das Register kann 65536 Werte
	// halten, wir m�ssen also ungef�hr um den Faktor 10 reduzieren. Teiler 8
	// w�re prima (dann m�sste man 80k-Zyklen haben).
	// Mit 65530 Zyklen w�re das eine Wiederholrate von 16,4ms - das ist prima.
	// 
	
	// Normalfrequenz 32 MHz mit DIV8 ergibt sich 4 MHz.
	tc->CTRLB = TC_WGMODE_SS_gc; // Waveform-Gen-Mode + CCAEN aktiviert Pin Override
	tc->PERL = 0xfa;
	tc->PERH = 0xff;
	tc->CTRLA = TC_CLKSEL_DIV8_gc;
}

void servo_setServoPosition(TC0_t * tc, unsigned char ccChannel, unsigned short int centiangle) {
	// F�r 1ms ist der Wert bei 4 MHz = 4000, f�r 2ms demnach 8000.
	unsigned short int val = (unsigned short int)(4000 + (((unsigned long int)centiangle /* * 4000*/) /* / 900 */)); // 125/8 <=> 4000/256
	// Berechnen wo f�r den Kanal die Register liegen:
	volatile unsigned char * regL = (unsigned char*)((unsigned short int)(&tc->CCAL) + (ccChannel * 2));
	volatile unsigned char * regH = (unsigned char*)((unsigned short int)(&tc->CCAH) + (ccChannel * 2));
	// Den entsprechenden Kanal f�r PWM aktivieren - dadurch wird der Pin �berschrieben:
	tc->CTRLB |= (TC0_CCAEN_bm << ccChannel);
	*regL = (unsigned char)val;
	*regH = (unsigned char)(val >> 8);
	//uart_printf_P(PSTR("Val %u."), val);
}
