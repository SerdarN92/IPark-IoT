#include <avr/io.h>
#include "common.h"
#include "hardware.h"
#include "system_clock.h"

void selectClockSource(unsigned char clockMode) {
	// Sicherstellen, dass der interne 2MHz-Oszillator läuft
	if (!(OSC.STATUS & OSC_RC2MRDY_bm)) {
		OSC.CTRL |= OSC_RC2MEN_bm;
		while (!(OSC.STATUS & OSC_RC2MRDY_bm)) { }
	}
	
	// Standard-Clock damit wir gefahrfrei die Register ändern können
	asm volatile ("ldi  r16,  0xD8");
	asm volatile ("out  0x34, r16");
	CLK.CTRL = CLK_SCLKSEL_RC2M_gc;
	
	switch (clockMode) {
		case SYSTEM_CLOCK_2MHz:
			// 2MHz is only clock source
			OSC.CTRL = OSC_RC2MEN_bm | OSC_RC32KEN_bm;
			asm volatile ("ldi  r16,  0xD8");
			asm volatile ("out  0x34, r16");
			CLK.PSCTRL = CLK_PSBCDIV_1_1_gc;
			break;
			
		case SYSTEM_CLOCK_32MHz:
			asm volatile ("ldi  r16,  0xD8");
			asm volatile ("out  0x34, r16");
			CLK.PSCTRL = CLK_PSBCDIV_1_1_gc;
			// 32MHz- und 32 kHz-Oszillatoren an (32 kHz nutzen wir zur Kalibrierung):
			OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;
			// Warten auf ext. Oszillator
			while (!(OSC.STATUS & OSC_RC32MRDY_bm) || !(OSC.STATUS & OSC_RC32KRDY_bm));
			// DFLL einschalten um den Oszillator besser zu kalibrieren:
			DFLLRC32M.CTRL = DFLL_ENABLE_bm;
			// Vorbereiten zum Umschalten der Clock:
			asm volatile ("ldi  r16,  0xD8");
			asm volatile ("out  0x34, r16");
			// Umschalten auf 32 MHz-Clock:
			CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
			break;
	}
}
