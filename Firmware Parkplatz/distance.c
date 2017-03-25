/*
 * distance.c
 *
 *  Author: thagemeier
 */ 
#include "hardware.h"
#include "distance.h"
#include "systimer.h"
#include <avr/interrupt.h>
#include "console.h"
#include "control.h"

static unsigned char distance;
static unsigned short int pwm_glob;
static unsigned char pwm_updated;

void dist_work(void * ptr);

void dist_init(void) {
	PORT_t * p;
	unsigned char pin;
	unsigned int portIndex;	
	
	// Setup RSSI measurements
	TCD1.CTRLB = TC0_CCAEN_bm;
	TCD1.CTRLD = TC_EVACT_PW_gc  | TC_EVSEL_CH0_gc;  // Event channel 0 for pulse width capture
	TCD1.CTRLE = 0;
	
	// Abrufen, welchen Pin wir überwachen müssen:
	hw_getPinData(DISTANCE_PWM_FUNCID, &p, 0, &pin);
	portIndex = (unsigned int)((unsigned int)p - (unsigned int)&PORTA) / (unsigned int)((unsigned int)&PORTB - (unsigned int)&PORTA);
	
	// Pin entsprechend einstellen. Wichtig hierbei: Es funktioniert nur BIS PORT F.
	EVSYS.CH0MUX = ((0b1010 + portIndex) << 3) | pin; // Port + Pin.
	uart_printf_P(PSTR("Value %u."), EVSYS.CH0MUX);
	EVSYS.CH0CTRL = 0;
	
	// Interrupts für Messung einschalten:
	TCD1.INTCTRLB = TC_CCAINTLVL_LO_gc;
	
	// Enable timer:
	TCD1.CTRLA = TC_CLKSEL_DIV64_gc; // Clock läuft mit 500 kHz
	
	// Regelmäßig Messungen durchführen:
	timer_addCallback(1, 1, dist_work, 0);
}

ISR(TCD1_CCA_vect) {
	unsigned int pwm_loc;
	// Hier lesen wir jetzt den letzten CCA-Wert aus.
	pwm_loc = TCD1.CCAL;
	pwm_loc |= ((unsigned int)TCD1.CCAH) << 8;
	pwm_glob = pwm_loc;
	pwm_updated = 1;
}

void dist_work(void * p) {
	unsigned char stmp;
	unsigned short int distance;
	unsigned char pwm_updated_loc;
	static unsigned short int mode;
	
	// Letzte Daten abrufen:
	stmp = SREG;
	cli();
	distance = pwm_glob;
	pwm_updated_loc = pwm_updated;
	pwm_updated = 0;
	SREG = stmp;
	
	if (pwm_updated_loc && (mode >= 21) && (mode <= 200) && (distance > 30)) { // >60 heißt > 13mm
		// Daten stehen zur Verfügung:
		// Der Sensor gibt bis 25ms aus oder 38ms für "keine Entfernung erfasst".
		// Da wir mit 500 kHz arbeiten, wäre der Grenzwert bei 30ms = 15000
		if (distance < 15000) {
			// In Millimeter umrechnen:
			distance = (distance * 10) / 29;
//			uart_printf_P(PSTR("(%umm) "), distance);
		} else {
			distance = 10000; // Mehr als der Sensor messen kann
//			uart_printf_P(PSTR("(Inf) "));
		}
		ctrl_sensorValue(0, distance);
		// Lesezyklus von vorne starten:
		mode = 250; // In 250ms + x neue Messung starten
	} else {
		switch (mode++) {
			case 20: // Messimpuls auslösen
				ON(DISTANCE_START_MEASURING);
				break;
			case 21: // 
				OFF(DISTANCE_START_MEASURING);
//				uart_printf_P(PSTR("+"));
				break;
				// Messimpuls kommt eigentlich nach einigen Mikrosekunden (fängt dann an)
			case 200: // Bis hierhin hätte eigentlich spätestens der Messimpuls zur Verfügung stehen sollen! Irgendwas bringt den Sensor durcheinander.. wir gehen davon aus es gibt _kein_ Hindernis
				distance = 0xffff;
				ctrl_sensorValue(0, 10000);
//				uart_printf_P(PSTR("(ERR) "));
				mode = 0; // Neue Messung probieren - Distanz ist ungültig
				break;
			case 500: // Neue Messung starten
				mode = 0;
				break;
		}
	}
}

unsigned int dist_getLastDistance() {
	return distance;
}
