#include <avr/interrupt.h>
#include "hardware.h"
#include "system_clock.h"
#include "sdram.h"
#include "systimer.h"
#include "uart_mgmt.h"
#include "coap_endpoints.h"
#include "settings.h"
#include "servo.h"
#include "common.h"
#include "rtc.h"
#include "distance.h"
#include "console.h"
#include "control.h"
#include "light.h"
#include "globals.h"

void timer_test(void * p) {
	//static unsigned char ctr;
	if (ISON(LED0)) {
		//OFF(LED0);
	} else {
		//ON(LED0);
	}
	// This is to test the UART but interferes with normal operation:
	//uart_putc(uartPC, ctr++);
}

void start_hardware() {
	// Pins initialisieren:
	hw_init();
	
	// Korrekte Clock ist wichtig:
	selectClockSource(SYSTEM_CLOCK_32MHz);
	
	// Interrupts einschalten:
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm; // | PMIC_RREN_bm; // Round robin-Interrupts machen das System noch schwerer einzuschätzen!
	// Interrupts benötigen wir überall!
	sei();
	
	// Externen RAM starten:
	sdram_init();

	// Systimer starten:
	systimer_init();
	
	// UART starten:
	uartmgmt_setupUARTs(hwDef.usartPC);
	
	// Parameter laden:
	set_init();
	
	// Wir haben nur eine interne 32 MHz-Clock die relativ ungenau ist. Wir brauchen aber genaues Timing
	// und benutzen dafür die RTC. Wir haben allerdings keine Referenz, von extern kann man die Unixtime
	// setzen und abfragen. Für die Parkzeitberechnung müssen wir das anders machen.
	rtc_init();
	
	// Kommunikation einrichten:
	com_init(uartPC);
	
	// CoAP-Endpoints einrichten:
	ep_setup();
	
	// Wir benutzen einen Servo an PORTC.0
	servo_init(&TCC0);
	
	// Distanzsensoren initialisieren:
	dist_init();

	// Ampel initialisieren:
	light_init();
	
	// Parkplatzkonstrolle EIN:
	control_init();

	timer_addCallback(10, 1, timer_test, 0);
}

int main() {
	
	start_hardware();
	
	while (1) {
		if (ISON(KEY3)) {
			// Über den Knopf können wir die Kollisionserkennung für eine sich hebende Sperre ganz abschalten falls das Probleme macht.
			globals.skipBarrierCollisionDetection = 1;
		}
		
		check_timers();
	}
	
	return 0;
}
