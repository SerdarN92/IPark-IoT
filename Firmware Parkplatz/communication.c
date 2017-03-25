#include "ringBuffer.h"
#include "uart.h"
#include "communication.h"
#include "extram.h"
#include "systimer.h"
#include "crc.h"
#include "settings.h"
#include "aes.h"
#include "common.h"
#include "coap_endpoints.h"
#include "hardware.h"
#include "console.h"
#include "uart_mgmt.h"
#include <string.h>

RB_BUFFER_t * inBuffer;
static unsigned char ringBufferBuffer[2000] EXTRAM;

#define COM_UART_IN_TIMEOUT_MS	15

void com_checkMessageBuffer(void * p);
void com_messageReceived(unsigned char * msg, unsigned short int len);
void com_protocolMessage(MESSAGE_t * msg);

static unsigned char in_timeout_ms;

// Diese Funktionen fügen Zeichen zum Ringpuffer hinzu wenn von der UART welche (jeweils ein Zeichen gleichzeitig) kommen
// Die zweite Funktion initialisiert nur die UART/den Ringpuffer

unsigned char com_uartEvent(void * infoPtr, unsigned char event, unsigned char data) {
	// So lange wie EVENT_RECEIVED das einzige ist wofür wir registriert sind ist das unnötig:
	//if (event == UART_EVENT_RECEIVED) {
		// Zeichen in Ringpuffer aufnehmen:
		rb_put(inBuffer, data);
		// Timeout zurücksetzen:
		in_timeout_ms = COM_UART_IN_TIMEOUT_MS;
	//}
	return 1;
}

void com_init(UART_INFO_t * uart) {
	// Puffer erstellen für eingehende Nachrichten:
	inBuffer = rb_createBuffer(&ringBufferBuffer[0], sizeof(ringBufferBuffer));
	
	if (inBuffer) {
		// Callback für UART registrieren, uns interessieren nur empfangene Bytes:
		uart_addModifyNotifyFunction(uart, inBuffer, com_uartEvent, UART_NOTIFY_RECEIVED);

		// Periodisch überprüfen ob wir neue Nachrichten empfangen haben:
		timer_addCallback(1, 1, com_checkMessageBuffer, 0);
	}
}

/*
** Folgende Funktionen werden aufgerufen um die Nachricht zu erkennen:
** - rb_delete_xx(inBuffer, x)		Löscht x Bytes aus dem Ringpuffer. x=0 -> Alles löschen
** - rb_peek_xx(inBuffer, x)		Liest Byte x aus dem Ringpuffer (0-basiert)
** - rb_getCount_xx(inBuffer)		Fragt die Anzahl Zeichen im Ringpuffer ab
** Die Implementierung des Ringpuffers für AVR ist in ringBuffer.c/h, da sind allerdings einige
** Architektur-Spezifische Sachen drin (wie z.B. cli()/sei() fürs Ein/Ausschalten der Interrupts)
*/

// Funktion wird regelmäßig aufgerufen (1ms) und prüft ob im Ringpuffer eine korrekte Nachricht enthalten ist.
// Bei einer Portierung auf x86 muss die Zeitbehandlung (letzte 15 Zeilen) geändert werden. Die Funktion
// soll, so lange keine gültige Nachricht vorliegt und mindestens 15ms kein Zeichen rein gekommen ist (Timeout
// muss man auf dem PC eventuell anpassen) das falsch erkannte Startbyte was am Anfang im Ringpuffer steckt
// wegwerfen.
void com_checkMessageBuffer(void * p) {
	
	unsigned short int rxc;
	unsigned short int msglen;
	unsigned short int recvd_crc;
	unsigned short int mycrc;
	unsigned short int i;
	static unsigned char running;
	
	// Timer-Funktionen sind eigentlich reentrant. Das müssen wir hier verhindern. Da
	// wir kein Multithreading haben reicht diese Konstruktion vollkommen aus:
	if (running)
		return;
	running = 1;
	
	// So lange der Puffer nicht leer ist..
	while ((rxc = rb_getCount_sync(inBuffer))) {
		// .. gucken wir ob wir ein SYNC empfangen haben:
		if (SYNCBYTE == rb_peek_sync(inBuffer, 0)) {
			// Danach kommt die Länge der Information, diese ist begrenzt auf 0x40-Bytes (64 Bytes). Bei längeren Sequenzen besteht
			// bei einer Fehlerkennung des Sync-Bytes sonst die Gefahr, das das Modul relativ lange darauf wartet, weitere
			// Informationen zu bekommen und daher wichtige Befehle verpasst..
			if (rxc > 4) { // 5 Zeichen sind das Minimum für einen echten Befehl (SYNC, Länge x 2, Content, Checksumme x 2)
				msglen = ((unsigned short int)rb_peek_sync(inBuffer, 1) << 8) | rb_peek_sync(inBuffer, 2);
				if (msglen > 0 && (msglen <= MAXMESSAGELENGTH)) {
					if ((unsigned char)(msglen + 5) <= rxc) { // Die 5 Bytes SYNCBYTE, LÄNGE x 2, CRC x 2 noch einrechnen
						
						// Zuerst die komplette CRC berechnen. Wenn wir einem unechten SYNC aufgesessen sind,
						// sollte das verhindern, das zu viele der echten Zeichen von der Adresserkennung
						// weggeworfen werden. Auf diese Art kann der dahinter kommende, "echte" Befehl
						// noch ausgewertet werden:
						// Beispiel für Nachricht mit Länge 2: 0) START, 1) LEN_H, 2) LEN_L, 3) Data1, 4) Data2, 5) CRC_H, 6) CRC_L
						// -> msglen ist dann 2, letztes i soll 4 sein
						mycrc = 0xffff;
						for (i=1;i<(msglen+3);i++) //+3, da SYNC und LEN_H/LEN_L nie zur Länge addiert wurden sondern nur in der IF-Abfrage oben mit +2 (+1) berücksichtigt sind..
							mycrc = crc_calcCRC16r(mycrc, rb_peek_sync(inBuffer, i));
						
						// Empfangene CRC:
						recvd_crc = ((unsigned short int)rb_peek_sync(inBuffer, msglen + 3) << 8) | rb_peek_sync(inBuffer, msglen + 4);
						
						// Prüfen auf Übereinstimmung:
						if (mycrc == recvd_crc) {
							// TODO: Sollten wir an dieser Stelle schon prüfen ob wir die Nachricht wegen der Zieladresse auswerten wollen..?
							// Das würde eigentlich Sinn machen, es spart uns das umkopieren der Nachricht in den linearen Puffer.
							
							// Nachricht in linearen Puffer kopieren (macht die Sache für die auswertenden Funktionen einfacher):
							unsigned char databytes[msglen];
							for (i=0;i<msglen;i++)
								databytes[i] = rb_peek_sync(inBuffer, 3 + i);
							
							com_messageReceived(&databytes[0], msglen);
							
							// Die Nachricht aus dem Puffer entsorgen:
							rb_delete_sync(inBuffer, msglen + 5);
							
							// Keine Nachricht mehr:
							msglen = 0;
						} else {
							// Die CRC war falsch. Eventuell war das garkeine Nachricht auf die wir reagiert haben!
							// Also einfach das "falsche" SYNC wegwerfen und nach einem neuen SYNC suchen:
							rb_delete_sync(inBuffer, 1);
						}
						// Nächsten Versuch starten:
						continue;
					}
				} else {
					// Das gedachte Sync-Zeichen war kein Sync!
					rb_delete_sync(inBuffer, 1);
					continue;
				}
			}
			// Der Befehl ist noch nicht vollständig. Wenn aber die einzelnen Zeichen einer Nachricht mehr als (X)ms auseinander liegen,
			// wird sie abgebrochen da wir evtl ein nicht-sync-Zeichen als SYNC interpretiert haben weil wir
			// den Anfang der echten Nachricht nicht mitbekommen haben. In dem Fall übergehen wir das SYNC-Zeichen und laufen direkt zum
			// nächsten SYNC-Byte.
			if (!in_timeout_ms) {
				// SYNC wegwerfen:
				rb_delete_sync(inBuffer, 1);
				continue; // Und gleich beim nächsten Byte gucken ob das ein SYNC-Byte sein könnte..
			} else {
				// Wir laufen einmal pro Millisekunde. Perfekt also um den Zähler runter zu zählen wenn das nötig ist.
				in_timeout_ms--; // Tick, tick..
				break;
			}
		} else {
			// Zeichen wird verworfen:
			rb_delete_sync(inBuffer, 1);
		}
	}
	
	running = 0;
}

void com_messageReceived(unsigned char * bytes, unsigned short int len) {
	unsigned char * ptr = bytes;
	
	if (len > 12) {
		// msg[0] - Destination
		// msg[4] - Source
		// msg[8] - Flags
		// msg[9] - Subprotocol
		// msg[10..15] - Sequence
		MESSAGE_t msg;
		msg.msgDest = *((unsigned long int*)&bytes[0]);
		// Convert to LE:
		reverseByteOrder(&msg.msgDest, 4);
		if (settings.networkAddress == msg.msgDest) {
			// Extract source:
			msg.msgSrc = *((unsigned long int*)&bytes[4]);
			// Convert to LE:
			reverseByteOrder(&msg.msgSrc, 4);
#ifdef USE_PORT_NUMBERS
			// Extract port 1:
			msg.dstPort = *((unsigned short int*)&bytes[8]);
			// Convert to LE:
			reverseByteOrder(&msg.dstPort, 2);
			// Extract port 2:
			msg.srcPort = *((unsigned short int*)&bytes[10]);
			// Convert to LE:
			reverseByteOrder(&msg.srcPort, 2);
			
			// Get flag:
			msg.flags = bytes[12];
			
			// Offset for optional fields:
			ptr = &bytes[13];
			len -= 13;
#else
			msg.flags = bytes[8];
			ptr = &bytes[9];
			len -= 9;
#endif			
			if (len && (msg.flags & MSG_FLAG_SUBPROTOCOL)) {
				msg.subProtocol = *ptr++;
				len--;
			} else {
				msg.subProtocol = 0;
			}
			if (msg.flags & MSG_FLAG_ENCRYPTED) {
				if (len > 6) {
					// Diese Sequenznummer wurde gesendet:
					msg.sequence = ptr;
					
					aes_encryptDecryptData(&ptr[6], len - 6, &bytes[0], &bytes[4], &ptr[0]); // len - 6 da auch die abschließenden CRC-Bytes verschlüsselt sind!
					// ptr auf die eigentlichen Daten setzen:
					ptr += 6;
					len -= 6;
					// An ursprünglich verschlüsselten Daten ist eine neue CRC angefügt um zu
					// testen ob die Entschlüsselung korrekt war. Das machen wir jetzt:
					unsigned short int crc = 0xffff;
					unsigned short int pos;
					for (pos=0;pos<len;pos++)
						crc = crc_calcCRC16r(crc, ptr[pos]);
					if (crc == ((ptr[len-2] << 8) | (ptr[len-1]))) {
						msg.content = &ptr[0];
						msg.len = len - 2;
						
						// Eine Nachricht wurde empfangen. Falls sie verschlüsselt war, müssen wir testen ob die Nachricht nicht zu alt ist (Sequenznummer darf nicht "verbraucht" sein):
						pos = 6;
						while (pos--) {
							if (msg.sequence[pos] < settings.maxKnownSequence[pos]) {
								// Notify the sender of the low sequence number:
								MESSAGE_t reply;
								reply.msgDest = msg.msgSrc;
								reply.srcPort = msg.dstPort;
								reply.dstPort = msg.srcPort;
								reply.flags = MSG_FLAG_ENCRYPTED | MSG_FLAG_SUBPROTOCOL;
								reply.sequence = &settings.maxKnownSequence[0];
								reply.subProtocol = 0xff; // "Marker-Protokoll" das die Sequenznummer zu niedrig war
								reply.content = (unsigned char*)"SEQNUM"; // Content muss vom Empfänger überprüft werden! Sonst könnte jemand die Sequenznummer illegal erhöhen.
								reply.len = 6;
								com_sendProtocolMessage(uartPC, &reply);
								settings.modified = 1;
								return;
							}
							// Wenn wir beim letzten Durchlauf 
							if (msg.sequence[pos] > settings.maxKnownSequence[pos]) {
								// Die gesendete Sequenznummer ist größer als die letzte gespeicherte. Damit müssen wir das EEPROM gleich updaten.
								// Im Fallthrough-Case müssen wir das nicht unbedingt (wenn wir antworten erhöhen wir die SeqNum und setzen da dann
								// modified-Marker sowieso).
								settings.modified = 1;
								break;
							}
						}
						// Okay, wir sind mindestens bei der gleichen Sequenznummer
						memcpy(&settings.maxKnownSequence[0], msg.sequence, sizeof(settings.maxKnownSequence));
						
						// Paket wurde offenbar korrekt dekodiert und gilt damit als empfangen. Weiter reichen an den Protokollstack der sich darum kümmert:
						com_protocolMessage(&msg);
					}
				}
			} else {
				msg.content = &ptr[0];
				msg.len = len;
				com_protocolMessage(&msg);
			}
		}
	}
}

void com_protocolMessage(MESSAGE_t * msg) {
	
	
	// Ein komplettes Paket wurde empfangen. An CoAP oder sonst wen weiter geben (abhängig vom Sub-Protokoll)
	if (msg->subProtocol == 0) {
		// An CoAP-Stack weiter geben, Quell- und Zieladresse stehen in msg. Portnummern gibts nicht, die sollten wir "hart" auf irgendeinen Wert setzen um CoAP das vorzugaukeln..
		ce_processIncomingMessage(msg);
	} else {
		// TODO: Hier kann später noch was anderes kommen, z.B. Config-Einstellungen oder so
	}
}


void increaseSequenceNumber(unsigned char * seqNum);

void com_sendProtocolMessage(UART_INFO_t * uart, MESSAGE_t * msg) {
	// Pointer auf die Nachrichtendaten:
	unsigned char * toSend = msg->content;
	unsigned short int len = msg->len;
	
	// Eventuell verschlüsseln wir. Dafür brauchen wir genug Platz im Puffer:
	unsigned char data[sizeof(MESSAGE_t)+len+2/*Extra CRC*/+6/* SeqNum */]; // Platz für den Worst Case (Verschlüsselt mit Unterprotokoll). Startbyte und Länge senden wir fix
	unsigned char * ptr1 = &data[2]; // Wir lassen die Bytes 0 und 1 aus, da kommt später noch die Länge der Nachricht rein
	unsigned char * ptr2 = &msg->sequence[0];
	unsigned short int i;
	unsigned short int crc = 0xffff;
	MESSAGE_t msgNew = *msg;
	
	// Nachrichten gehen immer von uns aus:
	msgNew.msgSrc = settings.networkAddress;
	
	// Korrekte Endiannes bei Quelle/Ziel und Ports herstellen:
	reverseByteOrder(&msgNew.msgSrc, 4);
	reverseByteOrder(&msgNew.msgDest, 4);
	reverseByteOrder(&msgNew.srcPort, 2);
	reverseByteOrder(&msgNew.dstPort, 2);
	
	// Destination address
	i = 4;
	ptr2 = (unsigned char*)&msgNew.msgDest;
	while (i--)
		*ptr1++ = *ptr2++;
	
	// Source address
	i = 4;
	ptr2 = (unsigned char*)&msgNew.msgSrc;
	while (i--)
		*ptr1++ = *ptr2++;
#ifdef USE_PORT_NUMBERS	
	// Destination port:
	i = 2;
	ptr2 = (unsigned char*)&msgNew.dstPort;
	while (i--)
		*ptr1++ = *ptr2++;
	
	// Source port:
	i = 2;
	ptr2 = (unsigned char*)&msgNew.srcPort;
	while (i--)
		*ptr1++ = *ptr2++;
#endif
	
	// Wenn wir verschlüsseln sollen brauchen wir dafür eine Sequenznummer!
	if ((msgNew.flags & MSG_FLAG_ENCRYPTED) && (!msgNew.sequence)) {
		ON(LED1);
		msgNew.flags &= ~MSG_FLAG_ENCRYPTED;
	}
	
	// Flag-Byte:
	*ptr1++ = msgNew.flags;
	
	if (msgNew.flags & MSG_FLAG_SUBPROTOCOL)
		*ptr1++ = msgNew.subProtocol;
	
	// Bei Verschlüsselung müssen wir die Daten erst verschlüsseln und dann in der Nachricht austauschen:
	if (msgNew.flags & MSG_FLAG_ENCRYPTED) {
		// Sequenznummer einbauen:
		ptr2 = &msgNew.sequence[0];
		i = 6;
		while (i--)
			*ptr1++ = *ptr2++;
		unsigned char * startOfEncryption;
		startOfEncryption = ptr1;
		// Daten kopieren und CRC berechnen:
		ptr2 = toSend;
		i = len;
		while (i--) {
			crc = crc_calcCRC16r(crc, *ptr2);
			*ptr1++ = *ptr2++;
		}
		// Was kommt durch die Verschlüsselung hinzu? Die Sequenznummer und am Ende nochmal eine 16 Bit-CRC
		// CRC anhängen:
		*ptr1++ = (unsigned char)(crc >> 8);
		*ptr1++ = (unsigned char)crc;
		// Inhalt verschlüsseln:
		aes_encryptDecryptData(startOfEncryption, len + 2, (unsigned char*)&msgNew.msgSrc, (unsigned char*)&msgNew.msgDest, &msgNew.sequence[0]);
		// Okay, wir senden statt der ursprünglichen Daten die verschlüsselte Version:
		toSend = &data[0];
		len+=8;
		// Im Anschluss müssen wir die Sequenznummer erhöhen damit wir nächstes mal entweder mit der neuen Sequenznummer verschlüsseln aber auch nichts kleineres mehr akzeptieren:
		increaseSequenceNumber(msgNew.sequence);
	} else {
		ptr2 = toSend;
		i = len;
		while (i--)
			*ptr1++ = *ptr2++;
	}
	
	// Das war ursprünglich nur die Länge der eigentlichen Daten. Korrigieren:
	len = ((unsigned int)ptr1 - (unsigned int)&data[0]);
	
	data[0] = (unsigned char)((len-2) >> 8); // 2 abziehen, die Längen-Bytes dürfen wir im fertigen Frame für die Nachrichtenlänge nicht mit angeben
	data[1] = (unsigned char)(len-2);
	
	// Nachricht senden und in einem Rutsch die Prüfsumme berechnen. AB HIER muss das portiert werden so dass die Bytes die per "uart_putc" raus
	// geschickt werden auf der UART landen.
	uart_putc(uart, SYNCBYTE);
	
	crc = 0xffff;
	for (i=0;i<len;i++) {
		crc = crc_calcCRC16r(crc, data[i]);
		uart_putc(uart, data[i]);
	}
	// Prüfsumme fehlt noch:
	uart_putc(uart, (unsigned char)(crc >> 8));
	uart_putc(uart, (unsigned char)crc);
	
	// Fertig! Die Verwaltung der Sequenznummer kommt später
}

void increaseSequenceNumber(unsigned char * seqNum) {
	unsigned char i = 6;
	for (i=0;i<6;i++) {
		if (++seqNum[i]) // Rolling counter. Overflow => value 0 => NO break => Increase next index
			return;
	}
	// Wenn wir bis hierhin kommen hatten wir einen Überlauf der Sequenznummer insgesamt. Das ist ganz schlecht!
}

/*

typedef struct {
	unsigned long int msgDest;
	unsigned long int msgSrc;
	unsigned char flags;
	unsigned char sequence[6];
	unsigned char subProtocol;
	unsigned char content;
	unsigned short int len;
} MESSAGE_t;

Nachrichtenformat:
- Zieladresse 4 Byte
- Quelladresse 4 Byte
- Flags 1 Byte
- Subprotokoll 1 Byte (falls in Flags spezifiziert)
- IV 6 Byte [nur wenn Flag signalisiert das verschlüsselt wurde)
- Verschlüsselte Daten
- Verschlüsselte CRC16 über die Daten

Flags:
- Bit 1: Verschlüsselt
- Bit 0: Subprotokoll angegeben (nicht angegeben: Subprotokoll = 0)

*/
