/*
 * uart.c
 *
 *  Author: thagemeier
 */ 

#include "hardware.h"
#include "ringBuffer.h"
#include "uart.h"
#include "systimer.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "common.h"

// Ungefähr 1,5 Byte-Längen müssen wir rein kommende Zeichen ignorieren. Der Receiver erkennt im Worst-Case ab dem Ende des Zeichens
// (was wir gerade gesendet haben) den Anfang des Echos. So lange brauchen wir also auf jeden Fall!
#define SUPPRESS_ECHO_DELAY			30		// ACHTUNG: DAS FUNKTIONIERT VERMUTLICH NUR MIT 57600 BAUD.

/*
- Basisadresse der UART (Pointer auf Objekt)		16 Bit
- Leseindex TX						16 Bit
- Schreibindex TX					16 Bit
- Anzahl TX						16 Bit
- Leseindex RX						16 Bit
- Schreibindex RX					16 Bit
- Anzahl RX						16 Bit
- Config-Byte						 8 Bit (Echo ja/nein, check ja/nein, empty on silence)
- Exklusivzugriff für ..				 8 Bit (Jede Funktion kann Exklusivzugriff "beantragen")
Wenn auf den FT über den Bus zugegriffen werden
muss, muss dies geschehen indem zuerst alle der
Exklusivzugriff aufgegeben wird (incl leeren des
Puffers) und dann der FT angesprochen wird. Zum
zurück senden der Nachricht muss wieder Exklusiv-
zugriff auf den Bus bestehen.
*/

static volatile unsigned char uartTestState;
static volatile unsigned char uartTestData;

UART_INFO_t * uart_createUART(USART_t * usart, unsigned char flags, void * UARTStructBuffer, unsigned char uartStructSize, void * bufferSpace, unsigned short int bufferSize) {
	UART_INFO_t * uart = (UART_INFO_t*)UARTStructBuffer;
	RB_BUFFER_t * buf;
	
	// Wenn wir zu wenig Platz haben -> doof
	if ((sizeof(UART_INFO_t) + sizeof(UART_NotifyInfo_t)) > uartStructSize)
		return (UART_INFO_t*)0;
	
	{
		unsigned int len = uartStructSize;
		unsigned char * ptr = (unsigned char*)UARTStructBuffer;
		while (len--)
			*ptr++ = 0;
	}
	{
		unsigned int len = bufferSize;
		unsigned char * ptr = (unsigned char*)bufferSpace;
		while (len--)
			*ptr++ = 0;
	}
	
	buf = rb_createBuffer(bufferSpace, bufferSize);
	if (!buf)
		return (UART_INFO_t*)0;
	
	uart->usart = usart;
	uart->outputBuffer = buf;
	uart->exclusive = 0;
	uart->exclusiveCount = 0;
	uart->flags = flags;
	uart->notifyCount = (uartStructSize - sizeof(UART_INFO_t)) / sizeof(UART_NotifyInfo_t);
	uart->stateReceive = UART_STATE_NOINIT;
	uart->stateSend = UART_STATE_NOINIT;
	
	return uart;
}

unsigned char uart_addModifyNotifyFunction(UART_INFO_t * uart, void * infoPtr, UART_notifyFunction f, unsigned int notifyMask) {
	unsigned char i;
	for (i=0;i<uart->notifyCount;i++) {
		if (uart->notifyInfo[i].f == (UART_notifyFunction)f) {
			uart->notifyInfo[i].notifyMask = notifyMask;
			uart->notifyInfo[i].infoPtr = infoPtr;
			return 1;
		}
	}
	for (i=0;i<uart->notifyCount;i++) {
		if (!uart->notifyInfo[i].f) {
			uart->notifyInfo[i].f = (UART_notifyFunction)f;
			uart->notifyInfo[i].notifyMask = notifyMask;
			uart->notifyInfo[i].infoPtr = infoPtr;
			return 1;
		}
	}
	return 0;
}

static void uart_doNotify(UART_INFO_t * uart, unsigned char event, unsigned char data) {
	for (unsigned char i=0;i<uart->notifyCount;i++) {
		if (uart->notifyInfo[i].notifyMask & (1 << event)) { // Diese Funktion hat Interesse an dem Event..? Dann sagen wir bescheid und die Funktion darf entscheiden ob noch weitere Interessenten die Info bekommen
			UART_notifyFunction f = (UART_notifyFunction)uart->notifyInfo[i].f;
			if (!f(uart->notifyInfo[i].infoPtr, event, data))
				break;
		}
	}
}

// Forward to following listener:
void uart_forwardNotify(UART_INFO_t * uart, unsigned char event, unsigned char data, void * afterPtr) {
	unsigned char fromHere = 0;
	for (unsigned char i=0;i<uart->notifyCount;i++) {
		if (fromHere) {
			if (uart->notifyInfo[i].notifyMask & (1 << event)) { // Diese Funktion hat Interesse an dem Event..? Dann sagen wir bescheid und die Funktion darf entscheiden ob noch weitere Interessenten die Info bekommen
				UART_notifyFunction f = (UART_notifyFunction)uart->notifyInfo[i].f;
				if (!f(uart->notifyInfo[i].infoPtr, event, data))
					break;
			}
		} else if (uart->notifyInfo[i].f == afterPtr) {
			fromHere = 1;
		}
	}
}

void uart_receivedInterrupt(UART_INFO_t * uart) {
	unsigned char state;
	unsigned char data;
	unsigned char checkByte;
	
	state = uart->usart->STATUS; // Status der UART ZUERST abrufen (Frame error etc)
	data = uart->usart->DATA; // Muss IMMER abgerufen werden um Interrupt zu löschen!;
	
	// Support echo test:
	if (uart->flags & UART_FLAG_PERFORM_ECHO_TEST) {
		uartTestState = state;
		uartTestData = data;
		uart->flags &= ~UART_FLAG_PERFORM_ECHO_TEST;
	}
	
	if ((uart->flags & UART_FLAG_CANCEL_ECHO) && uart->readAhead) { // Wenn wir überhaupt eingehende Daten prüfen wollen..
		checkByte = rb_peek(uart->outputBuffer, 0); // Byte abrufen
		uart->readAhead--;
		if (checkByte != data) {
			uart_doNotify(uart, UART_EVENT_COLLISION, data);
		}
	} else if (uart->flags & UART_FLAG_IGNORE_ECHO_EX) {
		// Hier prüfen ob das Byte ignoriert werden muss
		if (uart->doCancelEcho) {
			if ((timer_getPreciseTimestamp_async() - uart->lastEchoByteTime) > SUPPRESS_ECHO_DELAY) {
				uart->doCancelEcho = 0; // Okay, ab hier wieder Normalmodus. Wir lassen das Zeichen durch.
			} else {
				return; // Dieses Zeichen muss ignoriert werden.
			}
		}
	}
	if (state & (1 << 4)) { // Wenn wir einen frame error haben, haben wir kein normales Zeichen empfangen sondern einen frame error.
		uart_doNotify(uart, UART_EVENT_FRAME_ERROR, data);
	} else {
		// Alle Unterfunktionen die von dieser UART empfangen benachrichtigen. Puffer werden jeweils nur für die Eingabe-Funktionen bereit gehalten.
		uart_doNotify(uart, UART_EVENT_RECEIVED, data);
	}
}

void uart_dataEmptyInterrupt(UART_INFO_t * uart) {
	unsigned char data;
	if (uart->outputBuffer->size) {
		// Zustand: Wir senden
		uart->stateSend = UART_STATE_SENDING;
		// Daten aus dem Puffer lesen aber drin lassen (gelöscht werden sie, wenn wir den Eingang der Daten geprüft haben und das gleiche rein gekommen ist wie raus gegangen):
		if (uart->flags & UART_FLAG_CANCEL_ECHO) {
			data = rb_peek(uart->outputBuffer, uart->readAhead);
			uart->readAhead++;
		} else if (uart->flags & (UART_FLAG_IGNORE_ECHO | UART_FLAG_IGNORE_ECHO_EX)) {
			// Receiver so lange abschalten:
			uart->usart->CTRLB &= ~USART_RXEN_bm;
			// Daten senden:
			data = rb_get(uart->outputBuffer);
		} else {
			data = rb_get(uart->outputBuffer);
		}
		// Anfangen, die Daten zu senden:
		uart->usart->DATA = data;
	} else {
		if (uart->stateSend == UART_STATE_SENDING)
			uart->stateSend = UART_STATE_WAITING;
		// Interrupt für DRE abschalten:
		uart->usart->CTRLA &= ~USART_DREINTLVL_gm;
	}
}

void uart_txCompleteInterrupt(UART_INFO_t * uart) {
	uart->stateSend = UART_STATE_IDLE;
	uart_doNotify(uart, UART_EVENT_SENT, 0);
	if (uart->flags & (UART_FLAG_IGNORE_ECHO | UART_FLAG_IGNORE_ECHO_EX)) {
		// Receiver wieder einschalten:
		uart->usart->CTRLB |= USART_RXEN_bm;
	}
	if (uart->flags & UART_FLAG_IGNORE_ECHO_EX) {
		// Timeout einleiten wann wir wieder Bytes annehmen
		uart->doCancelEcho = 1;
		uart->lastEchoByteTime = timer_getPreciseTimestamp_async();
	}
}

unsigned char uart_getExclusiveAccess(UART_INFO_t * uart, unsigned char ID) {
	cli();
	if (!uart->exclusiveCount || (uart->exclusive == ID)) {
		uart->exclusive = ID;
		uart->exclusiveCount++;
		sei();
		return 1;
	} else {
		sei();
		return 0;
	}
}

void uart_releaseExclusiveAccess(UART_INFO_t * uart, unsigned char ID) {
	cli();
	if (uart->exclusive == ID) {
		uart->exclusiveCount--;
		if (!uart->exclusiveCount)
			uart->exclusive = 0;
	}
	sei();
}

unsigned char uart_hasExclusiveAccess(UART_INFO_t * uart, unsigned char ID) {
	return (uart->exclusive == ID) ? 1 : 0;
}

unsigned char uart_putc_async(UART_INFO_t * uart, unsigned char character) {
	rb_put(uart->outputBuffer, character);
	uart->usart->CTRLA |= USART_DREINTLVL_LO_gc;
	return 1;
}

unsigned char uart_putc(UART_INFO_t * uart, unsigned char character) {
	unsigned char res = rb_put_sync(uart->outputBuffer, character);
	uart->usart->CTRLA |= USART_DREINTLVL_LO_gc;
	return res;
}

unsigned char uart_puts(UART_INFO_t * uart, void * buffer, unsigned char buflen) {
	unsigned char res = 1;
	unsigned char * buf = (unsigned char*)buffer;
	while (buflen--)
		res &= rb_put_sync(uart->outputBuffer, *buf++);
	uart->usart->CTRLA |= USART_DREINTLVL_LO_gc;
	return res;
}

unsigned char uart_puts_P(UART_INFO_t * uart, void * buffer, unsigned char buflen) {
	unsigned char res = 1;
	unsigned char * buf = (unsigned char*)buffer;
	while (buflen--)
		res &= rb_put_sync(uart->outputBuffer, pgm_read_byte(buf++));
	uart->usart->CTRLA |= USART_DREINTLVL_LO_gc;
	return res;
}

unsigned char uart_outputValue(UART_INFO_t * uart, unsigned int value) {
	unsigned char buf[10];
	unsigned char i = 0;
	unsigned char ok = 1;
	// Assemble string into number:
	do {
		buf[i++] = value % 10;
		value /= 10;
	} while (value);
	// Output in reverse order:
	while (i--)
		ok &= uart_putc(uart, buf[i] + '0');
	// Result:
	return ok;
}

void uart_enableHardware(UART_INFO_t * uart, unsigned long speedBaud, unsigned char flags) {
	unsigned int speedReg;
	USART_t * usart;
	
	// Ausgangspuffer löschen:
	rb_delete_sync(uart->outputBuffer, 0);
	
	// Geschwindigkeit berechnen und "readAhead" zurücksetzen
	speedReg = ((F_CPU / 8) / speedBaud);
	
	// Strukturdaten löschen:
	uart->readAhead = 0;
	uart->exclusive = 0;
	uart->exclusiveCount = 0;
	
	// Laut Datenblatt notwendig für die HW-Init:
	cli();
	usart = uart->usart;
	
	// USART initialisieren
	
	// Vorsichtshalber erstmal aus stellen:
	usart->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
	
	// USART 2x-Clock AUS:
	usart->CTRLB &= ~USART_CLK2X_bm;
	// USART auf asynchron:
	usart->CTRLC = (usart->CTRLC & ~USART_CMODE_gm) | USART_CMODE_ASYNCHRONOUS_gc;
	
	// USART-Geschwindigkeit:
	if (!(usart->CTRLB & USART_CLK2X_bm)) {
		speedReg >>= 1; // Nochmal durch 2 teilen wenn wir mit 8 mal lesen fahren.
	}
	speedReg--;
	usart->BAUDCTRLA = (unsigned char)speedReg;
	usart->BAUDCTRLB = (unsigned char)(speedReg >> 8);
	
	// Wortgröße setzen:
	usart->CTRLC = (usart->CTRLC & ~USART_CHSIZE_gm) | USART_CHSIZE_8BIT_gc; // | (flags ? USART_PMODE_EVEN_gc : 0);
	
	// Interrupt-Level setzen:
	usart->CTRLA = USART_RXCINTLVL_LO_gc | USART_TXCINTLVL_LO_gc | USART_DREINTLVL_OFF_gc;
	
	// RX und TX an:
	usart->CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
	
	// Okay, wir sind dann soweit:
	uart->stateSend = UART_STATE_IDLE;
	sei();
	
}

void uart_changeSpeed(UART_INFO_t * uart, unsigned long speedBaud) {
	unsigned int speedReg;
	USART_t * usart;
	
	// Geschwindigkeit berechnen und "readAhead" zurücksetzen
	speedReg = ((F_CPU / 8) / speedBaud);
	
	// Laut Datenblatt notwendig für die HW-Init:
	cli();
	usart = uart->usart;
	
	// USART initialisieren
	
	// Vorsichtshalber erstmal aus stellen:
	//usart->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
	
	// USART 2x-Clock AUS:
	usart->CTRLB &= ~USART_CLK2X_bm;
	
	// USART-Geschwindigkeit:
	if (!(usart->CTRLB & USART_CLK2X_bm)) {
		speedReg >>= 1; // Nochmal durch 2 teilen wenn wir mit 8 mal lesen fahren.
	}
	speedReg--;
	
	usart->BAUDCTRLA = (unsigned char)speedReg;
	usart->BAUDCTRLB = (unsigned char)(speedReg >> 8);
	
	// RX und TX an:
	//usart->CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
	
	sei();
}

void uart_disableHardware(UART_INFO_t * uart) {
	// UART de-initialisieren
	// Vorsichtshalber die globalen Interrupts deaktivieren..
	cli();
	// TXEN und RXEN, außerdem alle 3 möglichen USART-Interrupts (RX compl, TX compl und TX-buffer leer) _abschalten_!
	
	uart->usart->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
	uart->usart->CTRLA = USART_RXCINTLVL_OFF_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	
	// Merken, dass wir abgeschaltet haben:
	uart->stateSend = UART_STATE_NOINIT;
	
	// Interrupts wieder aktivieren
	sei();
}

unsigned char uart_isBusy(UART_INFO_t * uart) {
	return (uart->stateSend > UART_STATE_IDLE) ? 1 : 0;
}

unsigned int uart_getTransmissionDelayMs(unsigned long speedBaud, unsigned int rxlen) {
	// Wir gehen davon aus, dass wir immer 10 Bit pro Zeichen senden müssen.
	// Insgesamt ergibt sich daher für die Übertragungszeit:
	return (unsigned int)((((unsigned long int)rxlen * 100) / speedBaud + 5) / 10); // Wir runden korrekt
}

unsigned char uart_performEchoTest(UART_INFO_t * uart, unsigned char character) {
	unsigned char oldFlags;
	unsigned char timeout = 255;
	if (!uart)
		return 0;
	// Wait until buffer is empty:
	while (uart->outputBuffer->size) {
		if (!(--timeout))
			return 0;
		wait(1);
	}
	// Memorize old flags:
	oldFlags = uart->flags;
	// No echo suppression, instead perform ECHO TEST:
	uart->flags = UART_FLAG_PERFORM_ECHO_TEST;
	// Reset USART TEST status:
	uartTestState = 0;
	// Send out the test character:
	uart_putc(uart, character);
	// Bei 2400 Baud brauchen wir 4,5ms pro Zeichen:
	timeout = 10; // Nach 10ms muss spätestens eine Antwort da sein, sonst ist es kein Echo.
	// Wait for a short moment without letting others access the input buffer:
	while (!uartTestState) {
		if (!(--timeout))
			break;
		_delay_ms(1);
	}
	// Restore old flags:
	uart->flags = oldFlags;
	// Look at result, return 1 on ECHO SUCCESS.
	if (uartTestState && (character == uartTestData)) {
		return 1;
	} else {
		return 0;
	}
}
