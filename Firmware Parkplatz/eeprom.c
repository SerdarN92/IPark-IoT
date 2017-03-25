#include "eeprom.h"
#include <avr/io.h>
#include "systimer.h"
#include "common.h"
#include <avr/interrupt.h>
#include "memtransfer.h"
#include "hardware.h"

static unsigned int eew_address		= 0;
static unsigned char * eew_memPtr	= 0;
static unsigned int eew_dataLen		= 0;
static unsigned int eew_pageAddr	= 0;
static unsigned char * eew_saveTo	= 0;
static unsigned char * eew_readFrom	= 0;
static unsigned char eew_needWrite	= 0;
static volatile unsigned char eew_busy		= 0;
static volatile Void_Function_Void eeCallback;

// Make sure the rest doesn't get translated:
// NO_DYNAMIC_DATA -- BRAUCHEN WIR HIER NICHT, DA ASM JEWEILS MIT \ abschließt!
#define NVM_EXEC()	asm("push r30"      "\n\t"	\
			    "push r31"      "\n\t"	\
    			"push r16"      "\n\t"	\
    			"push r18"      "\n\t"	\
			    "ldi r30, 0xCB" "\n\t"	\
			    "ldi r31, 0x01" "\n\t"	\
			    "ldi r16, 0xD8" "\n\t"	\
			    "ldi r18, 0x01" "\n\t"	\
			    "out 0x34, r16" "\n\t"	\
			    "st Z, r18"	    "\n\t"	\
    			"pop r18"       "\n\t"	\
			    "pop r16"       "\n\t"	\
			    "pop r31"       "\n\t"	\
			    "pop r30"       "\n\t"	\
			    )

void ee_init() {
	// Enable EE-Mapped access
//	console_print_P(PSTR("Initializing EEPROM.. "));
	NVM.CTRLB |= NVM_EEMAPEN_bm;
//	console_print_success_P(PSTR("ok\r\n"));
}

unsigned char ee_saveData(unsigned int eeprom_address, void* data, unsigned int dataLen, unsigned char waitForEnd) {
	return ee_saveDataEx(eeprom_address, data, dataLen, waitForEnd);
}

unsigned char ee_isBusy(void) {
	return ((NVM.STATUS & NVM_NVMBUSY_bm) || eew_busy) ? 1 : 0;
}

unsigned char ee_saveDataEx(unsigned int eeprom_address, void* data, unsigned int dataLen, unsigned char waitForEnd) {
	// Wenn wir nicht warten, müssen wir evtl abbrechen:
	if (eew_busy && !waitForEnd)
		return 0;
	
	// Warten, dass die Async-Routine "frei" ist, müssen wir eh:
	while (eew_busy)
		check_timers();
	
	ee_asyncWrite(eeprom_address, data, dataLen);
	
	if (waitForEnd)	{ // Sollen wir auf das Ende warten?
		// Warten, bis alles vorbei ist:
		while (eew_busy)
			check_timers();
	}
	
	return 1;
}

void eew_feed(unsigned char inInt);

void ee_asyncWrite(unsigned int eeprom_address, void* data, unsigned int dataLen) {
	return ee_asyncWriteEx(eeprom_address, data, dataLen, 0);
}

void ee_asyncWriteEx(unsigned int eeprom_address, void* data, unsigned int dataLen, Void_Function_Void callback) {
	if (!dataLen)
		return;
	if (!eew_busy) {
		eeCallback = callback;
		eew_pageAddr = eeprom_address;
		eew_saveTo = (unsigned char*)(eeprom_address + MAPPED_EEPROM_START);
		eew_readFrom = (unsigned char*)data;
		eew_needWrite = 0;
		eew_address = eeprom_address;
		eew_memPtr = (unsigned char*)data;
		eew_dataLen = dataLen;
		// Start the whole process
		eew_busy = 1;
		eew_feed(0);
	}
}

void eew_feed(unsigned char inInt) {
	Void_Function_Void tmp = eeCallback;	
	
	if (eew_dataLen) { // Überhaupt noch Daten übrig..?
		// Interrupts aus:
		if (!inInt)
			cli();
		// Buffer sollte eigentlich nicht mit irgendwas gefüllt sein..
		if (NVM.STATUS & NVM_EELOAD_bm) {
			// Buffer löschen:
			NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
			NVM_EXEC();
			while (NVM.STATUS & NVM_NVMBUSY_bm);
		}
		
		while (eew_dataLen--) {												// So lange noch Daten übrig sind ..
			if (*eew_saveTo != *eew_readFrom) {								// Wenn die Daten an dieser Stelle unterschiedlich sind ..
				*eew_saveTo = *eew_readFrom;								// .. schreiben wir die Daten ..
				eew_needWrite = 1;											// .. und merken uns, dass wir schreiben müssen.
			}
			eew_saveTo++;													// Zum nächsten Byte gehen
			eew_readFrom++;													// .. auch bei den zu speichernden Daten.
			
			if (!(((unsigned int)eew_saveTo) & (EEPROM_PAGE_SIZE - 1)) ||	// Wenn wir das letzte Byte einer Seite geschrieben haben, speichern wir,
			    !eew_dataLen) {												// .. in jedem Fall jedoch, wenn es keine weiteren Daten mehr gibt.
				
				if (eew_needWrite) {										// Wenn wir Daten schreiben müssen, machen wir das.
					NVM.ADDR2 = 0;
					NVM.ADDR1 = (eew_pageAddr >> 8) & 0xff;
					NVM.ADDR0 = (eew_pageAddr & 0xff);
					// We are going to write stuff:
					eew_needWrite = 0;										// Wir haben fertig
					eew_pageAddr += EEPROM_PAGE_SIZE;						// Eine Seite weiter
					// Give write command:
					NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
					NVM_EXEC();
					// Enable Interrupt AFTER command to NVM:
					NVM.INTCTRL |= NVM_EELVL_MED_gc;
					
					if (!inInt)
						sei();
					return;													// Okay, we are done for now.
				} else {
					// Der Puffer wurde nie beschrieben, gleich weiter machen. Wir müssen aber trotzdem zur nächsten Seite:
					eew_pageAddr += EEPROM_PAGE_SIZE;
				}
			}
		}
		if (tmp) {
			eeCallback = 0;
			tmp();
		}
		if (!inInt)
			sei();
		eew_busy = 0; // Nichts mehr zu tun -> Wir mussten überhaupt nichts schreiben. Sonst wären wir ja oben per RETURN ausgestiegen.
	} else {
		if (tmp) {
			eeCallback = 0;
			tmp();
		}
		eew_busy = 0;
		// Done writing data
	}
}

ISR(NVM_EE_vect) { // __attribute__((section ("irq_handlers"))) {
	// Disable Interrupt:
	NVM.INTCTRL &= ~NVM_EELVL_gm;
	// Try feeding the next data sequence (interrupt might be re-enabled by this):
	eew_feed(1);
}

void ee_loadData(unsigned int eeprom_address, void* data, unsigned int dataLen) {
	unsigned char* eedata = (unsigned char*)(eeprom_address + MAPPED_EEPROM_START);
	unsigned char* eeto = data;
	unsigned int len = dataLen;
	while (NVM.STATUS & NVM_NVMBUSY_bm) { }
	while (len--)
		*eeto++ = *eedata++;
}
