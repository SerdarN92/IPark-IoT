#include "memtransfer.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

// Make sure the rest doesn't get translated:
// NO_DYNAMIC_DATA //


#define	__far_mem_copy(fromaddr, toaddr, length)										\
							asm volatile(												\
								"cli"										"\n\t"		\
								"push r28"									"\n\t"		\
								"push r29"									"\n\t"		\
								"in __tmp_reg__, %[rampy]"					"\n\t"		\
								"in __zero_reg__, %[rampz]"					"\n\t"		\
								"out %[rampy], %C0"							"\n\t"		\
								"out %[rampz], %C1"							"\n\t"		\
								"movw r28, %[from]"							"\n\t"		\
								"movw r30, %[to]"							"\n\t"		\
								"ld	r25, Y+"								"\n\t"		\
								"st Z+, r25"								"\n\t"		\
								"dec %[len]"								"\n\t"		\
								"brne .-8"									"\n\t"		\
								"out %[rampy], __tmp_reg__"					"\n\t"		\
								"out %[rampz], __zero_reg__"				"\n\t"		\
								"eor __zero_reg__, __zero_reg__"			"\n\t"		\
								"pop r29"									"\n\t"		\
								"pop r28"									"\n\t"		\
								"sei"										"\n\t"		\
								"ret"										"\n\t"		\
								:	/* No output operands */							\
								: [from] "r" (fromaddr), [to] "r" (toaddr), [len] "r" (length),	\
								  [rampy] "I" (_SFR_IO_ADDR(RAMPY)), [rampz] "I" (_SFR_IO_ADDR(RAMPZ))		\
								)

#define	__far_mem_clear(toaddr, length)													\
							asm volatile(												\
								"cli"										"\n\t"		\
								"in __tmp_reg__, %[rampz]"					"\n\t"		\
								"out %[rampz], %C0"							"\n\t"		\
								"movw r30, %[to]"							"\n\t"		\
								"eor __zero_reg__, __zero_reg__"			"\n\t"		\
								"st Z+, __zero_reg__"						"\n\t"		\
								"dec %[len]"								"\n\t"		\
								"brne .-8"									"\n\t"		\
								"out %[rampz], __tmp_reg__"					"\n\t"		\
								"sei"										"\n\t"		\
								"ret"										"\n\t"		\
								:	/* No output operands */							\
								: [to] "r" (toaddr), [len] "r" (length),				\
								  [rampz] "I" (_SFR_IO_ADDR(RAMPZ))						\
								)

void memoryTransferDMA(unsigned long from, unsigned long to, unsigned char howMany) {
	
	/*
	// DMA Channel 1 benutzen:
	DMA.CTRL |= DMA_ENABLE_bm | DMA_PRIMODE_CH0123_gc;
	
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;	
	*/
	
	// DMA konfigurieren:
	DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_INC_gc;
	DMA.CH1.TRIGSRC = 0;
	
	DMA.CH1.TRFCNTL = howMany;
	DMA.CH1.TRFCNTH = 0;
	
	DMA.CH1.REPCNT = 0;
	DMA.CH1.SRCADDR0 = (from & 0xff);
	DMA.CH1.SRCADDR1 = (from >> 8) & 0xff;
	DMA.CH1.SRCADDR2 = (from >> 16) & 0xff;
	DMA.CH1.DESTADDR0 = (to & 0xff);
	DMA.CH1.DESTADDR1 = (to >> 8) & 0xff;
	DMA.CH1.DESTADDR2 = (to >> 16) & 0xff;
	
	// Transfer starten:
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;
	DMA.CH1.CTRLA |= DMA_CH_TRFREQ_bm;
	
	// Warten, bis Transfer fertig.
	while (DMA.STATUS & (DMA_CH1BUSY_bm | DMA_CH1PEND_bm)) { }
	
}

void memoryTransfer(unsigned long from, unsigned long to, unsigned char howMany) {
	//memoryTransferDMA(from, to, howMany);
	if (howMany)
		__far_mem_copy(from, to, howMany);
}

void memoryTransferNoDMA(unsigned long from, unsigned long to, unsigned char howMany) {
	if (howMany)
		__far_mem_copy(from, to, howMany);
}

void memoryTransferLong(unsigned long from, unsigned long to, unsigned int howMany) {
	do {
		if (howMany > 128) {
			memoryTransfer(from, to, (unsigned char)128);
			//__far_mem_copy(from, to, 128);
			from += 128;
			to += 128;
			howMany -= 128;
		} else {
			memoryTransfer(from, to, (unsigned char)howMany);
			howMany = 0;
			break;
		}
	} while (howMany);
}


void memoryZero(unsigned long to, unsigned char howMany) {
	if (howMany)
		__far_mem_clear(to, howMany);
}

void memoryZeroLong(unsigned long to, unsigned int howMany) {
	do {
		if (howMany > 128) {
			memoryZero(to, 128);
			to += 128;
			howMany -= 128;
		} else {
			memoryZero(to, howMany);
			howMany = 0;
			//break;
		}
	} while (howMany);
}
