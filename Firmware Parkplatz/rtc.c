#include "rtc.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "hardware.h"
#include "accounting.h"

static unsigned long int ticks = 0;

void rtc_init() {
	// RTC aktivieren:
	CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;
	RTC.PERL = 0; // Wir teilen nicht runter, also sind wir bei 32768 = eine Sekunde.
	RTC.PERH = 0x80;
	RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;
	RTC.CTRL = RTC_PRESCALER_DIV1_gc;
}

ISR(RTC_OVF_vect) {
	ticks++;
	if (ISON(LED0)) {
		OFF(LED0);
	} else {
		ON(LED0);
	}
}

void rtc_setUnixtime(unsigned long int unixtime) {
	unsigned long int oldTime;
	unsigned char stmp;
	// Warten bis die RTC nicht mehr synchronisiert:
	while (RTC.STATUS & WDT_SYNCBUSY_bm);
	// So.. RTC-Wert updaten:
	RTC.CNTL = 0;
	RTC.CNTH = 0;	
	// Warten bis die RTC nicht mehr synchronisiert:
	while (RTC.STATUS & WDT_SYNCBUSY_bm);
	// Okay, jetzt hätten wir eine Sekunde Zeit die unixtime zu aktualisieren:
	stmp = SREG;
	cli();
	oldTime = ticks;
	ticks = unixtime;
	SREG = stmp;
	// Notify accounting of the new clock:
	accounting_setNewClock(oldTime, unixtime);
}

unsigned long int rtc_getUnixtime() {
	unsigned long int ltmp;
	unsigned char stmp = SREG;
	cli();
	ltmp = ticks;
	SREG = stmp;
	return ltmp;
}
