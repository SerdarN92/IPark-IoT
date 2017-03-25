#include "hardware.h"
#include "systimer.h"
#include "aes.h"
#include "common.h"
#include "settings.h"
#include "console.h"

void aes_encryptDecryptData(unsigned char * data, unsigned int len, unsigned char * msgSrc, unsigned char * msgDest, unsigned char * sequence) {
	unsigned short int s = 0;
	unsigned char x;
	unsigned char i = 0;
	//uart_printf_P(PSTR("Encrypting %.2x%.2x%.2x.."), data[0], data[1], data[2]);
	
	while (len--) {
		// AES_BUFFER_SIZE muss eine 2er-Potenz sein. Dann sind nämlich durch -1 alle niedrigeren Bits gesetzt und wir führen die Schleife bei allen Vielfachen von AES_BUFFER_SIZE aus.
		if (!(s & (AES_BUFFER_SIZE-1))) {
			// AES-Engine zurücksetzen:
			AES.CTRL = AES_RESET_bm;
			
			// AES.STATE ist der Datenblock, siehe Datenblatt.
			AES.STATE = msgSrc[0]; // Daten liegen zu diesem Zeitpunkt im Speicher in Big Endian (Network Byte Order) vor. Also alles korrekt, nichts umdrehen.
			AES.STATE = msgSrc[1];
			AES.STATE = msgSrc[2];
			AES.STATE = msgSrc[3];
			AES.STATE = msgDest[0];
			AES.STATE = msgDest[1];
			AES.STATE = msgDest[2];
			AES.STATE = msgDest[3];
			AES.STATE = sequence[0];
			AES.STATE = sequence[1];
			AES.STATE = sequence[2];
			AES.STATE = sequence[3];
			AES.STATE = sequence[4];
			AES.STATE = sequence[5];
			// Als letztes den Byte-Offset des Blocks in den Daten ("Counter Mode" für die Verschlüsselung - der lässt sich hinterher wunderbar parallelisieren)
			AES.STATE = (unsigned char)(s >> 8);
			AES.STATE = (unsigned char)s;
			// Pre Shared Key als eigentlicher Schlüssel:
			for (i=0;i<AES_BUFFER_SIZE;i++)
				AES.KEY = settings.preSharedKey[i];
			// Verschlüsseln:
			AES.CTRL = AES_START_bm;
			// Warten bis die AES-Engine fertig ist (dauert 375 Zyklen, bei 32 MHz sind das 11,7µs pro Block - darauf warten wir und erledigen keine anderen Aufgaben dazwischen):
			while (!(AES.STATUS & AES_SRIF_bm))
				asm volatile ("nop");
			wait(100);
		}
		
		// Abrufen des Streamsciphers:
		x = AES.STATE;
		//uart_printf_P(PSTR("%.2x "), x);
		*data++ ^= x;
		
		// Byte-Zähler weiter:
		s++;
	}
}































