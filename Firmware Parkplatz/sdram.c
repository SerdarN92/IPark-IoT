#include "hardware.h"
#include "common.h"
#include "systimer.h"
#define SDRAM_LINKERSCRIPT_USED
#include "sdram.h"
#include "memtransfer.h"
#include "extram.h"

//void sdram_init() __attribute__ ((naked, section (".init3")));

#ifdef EBI

extern unsigned int __extram_start;
extern unsigned int __extram_end;

static volatile unsigned char testBuffer[128] EXTRAM;
void sdram_testPeriodic(void * p);

unsigned char sdram_init() {
	unsigned char i;	
	unsigned char state = 1;
	//console_print("Setting up SDRAM.. ");
	
	/* Set signals which are active-low to high value */
	PORTH.OUT = 0x0F;
	
	/* Configure bus pins as outputs(except for data lines). */
	PORTH.DIR = 0xFF;
	PORTK.DIR = 0xFF;
	PORTJ.DIR = 0xF0;
	
	//PORTE.OUTCLR = 1; // Chip select für RAM -> AKTIV
	
	
	EBI.CTRL = 			EBI_SDDATAW_4BIT_gc | 		// 4-Bit-Interface
						EBI_IFMODE_3PORT_gc |		// 3 Ports
						EBI_SRMODE_NOALE_gc; 		// Eigentlich nicht relevant aber wir versuchen den RAM genau so anzusteuern wie Atmel es in der AppNote macht
	
	EBI.SDRAMCTRLA = 	EBI_SDROW_bm / 				// 12 Bit for Rows (4096)
						EBI_SDCOL_10BIT_gc | 		//  9 Bit for Columns (512)
						EBI_SDCAS_bm;				// CAS-Latenz 3 Zyklen
	
	// Refresh rate 14,91µS (minimal Rate specified by SDRAM 15,xx µS)
	EBI.REFRESH = 		440; //440; // WAS: 440 // Calculated for 28 MHz or 14 MHz with 2x peripheral clock
	
	// Initialization delay.. max. could be ~10ms
	EBI.INITDLY = 		7000; // Berechnet: 200µs = 5898 bei 29 MHz.
/*	
	EBI.SDRAMCTRLB =	EBI_MRDLY_2CLK_gc | 		// Mode->Active
						EBI_ROWCYCDLY_4CLK_gc | 	// Ausgerechnet von SELF REFRESH aus..
						EBI_RPDLY_2CLK_gc;			// Ausgerechnet
	EBI.SDRAMCTRLC =	EBI_WRDLY_2CLK_gc |			// Ausgerechnet
						EBI_ESRDLY_4CLK_gc |		// Ausgerechnet
						EBI_ROWCOLDLY_3CLK_gc;
	EBI.CS3.CTRLA =		EBI_CS_ASPACE_8MB_gc;
*/
/*	EBI.SDRAMCTRLB =	EBI_MRDLY_1CLK_gc | 		// 2CLK
						EBI_ROWCYCDLY_2CLK_gc | 	// 7CLK
						EBI_RPDLY_2CLK_gc;			// 7CLK
	EBI.SDRAMCTRLC =	EBI_WRDLY_2CLK_gc |			// Ausgerechnet
						EBI_ESRDLY_2CLK_gc |		// Ausgerechnet
						EBI_ROWCOLDLY_1CLK_gc;
*/
	
	
	EBI.SDRAMCTRLB =	EBI_MRDLY_2CLK_gc | 		// 2CLK
						EBI_ROWCYCDLY_7CLK_gc | 	// 7CLK
						EBI_RPDLY_7CLK_gc;			// 7CLK
	EBI.SDRAMCTRLC =	EBI_WRDLY_2CLK_gc |			// Ausgerechnet
						EBI_ESRDLY_7CLK_gc |		// Ausgerechnet
						EBI_ROWCOLDLY_7CLK_gc;
	
	EBI.CS3.CTRLA =		EBI_CS_ASPACE_8MB_gc;
	EBI.CS3.CTRLB =		0;
	
	EBI.CS3.BASEADDR =	0x0;						// Lowest SDRAM-addr >> 8, aligned to chip size (1M = 0x100000)
	
	
	
	EBI.CS3.CTRLA = EBI_CS_ASPACE_4MB_gc | EBI_CS_MODE_SDRAM_gc;
    while (!(EBI.CS3.CTRLB & EBI_CS_SDINITDONE_bm) );   // wait for init of SDRAM
	
	wait(200);
	
	//EBI.CS3.CTRLB = EBI_CS_SDMODE_LOAD_gc;
    //*((unsigned char*)0xf000) = 0x12; // dummy access to write MODE register
    //EBI.CS3.CTRLB = EBI_CS_SDMODE_NORMAL_gc;
	
	
	
	// DAS HIER MUSS ZULETZT DURCHGEFÜHRT WERDEN (MODE_SDRAM setzen)
//	EBI.CS3.CTRLA =		EBI_CS_ASPACE_8MB_gc |		// Erstmal.. später evtl erhöhen
//						EBI_CS_MODE_SDRAM_gc;
//	
//	while (!(EBI.CS3.CTRLB & EBI_CS_SDINITDONE_bm)) { }
	
	// Daten in den Testpuffer schreiben:
	i = sizeof(testBuffer);
	while (i--)
		testBuffer[i] = (i ^ 0xaa);
	
	// 25ms warten:
	wait(25);
	
	// Daten in den Testpuffer schreiben:
	i = sizeof(testBuffer);
	while (i--)
		testBuffer[i] = (i ^ 0xaa);
	
	// 25ms warten:
	wait(25);
	
	// Daten in den Testpuffer schreiben:
	i = sizeof(testBuffer);
	while (i--)
		testBuffer[i] = (i ^ 0xaa);
	
	// 25ms warten:
	wait(25);
	
	{
		unsigned char * p = (unsigned char*)&__extram_start;
		unsigned int len = ((unsigned int)&__extram_end - (unsigned int)&__extram_start);
		while (len--)
			*p++ = 0;
	}
	
	// RAM funktioniert offenbar erstmal:
	return state;
}

#endif






























































