/*

This file is based on a file provided by the microcoap library:

Copyright (c) 2013 Toby Jaffey <toby@1248.io>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
//#include <stdarg.h>
#include "coap.h"
#include "communication.h"
#include "extram.h"
#include "uart_mgmt.h"
#include "console.h"
#include "settings.h"
#include "globals.h"
#include "rtc.h"
#include "common.h"
#include "control.h"
#include "accounting.h"

static char light = '0';

const uint16_t rsplen = 1500;
static char rsp[1500] = "";

void build_rsp(void);
void ep_setup(void) { // CoAP-Endpoints initialisieren und registrieren:
    build_rsp();
}

// Für jedes Element ein GET/POST-Endpoint. Der "neue" void * info-Pointer verweist auf die ursprüngliche Nachricht,
// so das die Quelle herausgefunden werden kann oder ermittelt wird ob die Nachricht verschlüsselt war oder nicht.

static const coap_endpoint_path_t path_well_known_core = {2, {".well-known", "core"}};
static int handle_get_well_known_core(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	return coap_make_response(scratch, outpkt, (const uint8_t *)rsp, strlen(rsp), id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_APPLICATION_LINKFORMAT);
}

static const coap_endpoint_path_t path_light = {1, {"light"}};
static int handle_get_light(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	return coap_make_response(scratch, outpkt, (const uint8_t *)&light, 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static int handle_put_light(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	if (inpkt->payload.len == 0)
		return coap_make_response(scratch, outpkt, NULL, 0, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_BAD_REQUEST, COAP_CONTENTTYPE_TEXT_PLAIN);
	if (inpkt->payload.p[0] == '1') {
		light = '1';
		ON(LED0);
		ON(LED1);
		ON(LED2);
		ON(LED3);
		ON(LED4);
		ON(LED5);
		ON(LED6);
		ON(LED7);
		return coap_make_response(scratch, outpkt, (const uint8_t *)&light, 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CHANGED, COAP_CONTENTTYPE_TEXT_PLAIN);
	} else {
		light = '0';
		OFF(LED0);
		OFF(LED1);
		OFF(LED2);
		OFF(LED3);
		OFF(LED4);
		OFF(LED5);
		OFF(LED6);
		OFF(LED7);
		return coap_make_response(scratch, outpkt, (const uint8_t *)&light, 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CHANGED, COAP_CONTENTTYPE_TEXT_PLAIN);
	}
}

static const coap_endpoint_path_t path_barrier = {1, {"barrier"}};
static int handle_get_barrier(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	return coap_make_response(scratch, outpkt, (const uint8_t *)(ctrl_isBarrierEngaged() ? 'U' : 'D'), 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_cardetect = {1, {"car-detect"}};
static int handle_get_cardetect(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	return coap_make_response(scratch, outpkt, (const uint8_t *)(ctrl_isCarParked() ? "1" : "0"), 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_sequence = {1, {"sequence"}};
static int handle_get_sequence(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	char buffer[13];
	unsigned char len;
	len = snprintf(&buffer[0], sizeof(buffer), "%.2x%.2x%.2x%.2x%.2x%.2x", settings.maxKnownSequence[5], settings.maxKnownSequence[4], settings.maxKnownSequence[3], settings.maxKnownSequence[2], settings.maxKnownSequence[1], settings.maxKnownSequence[0]);
	return coap_make_response(scratch, outpkt, (const uint8_t *)&buffer[0], len, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_clock = {1, {"clock"}};
static int handle_get_clock(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	char buffer[11];
	unsigned char len;
	len = snprintf(&buffer[0], sizeof(buffer), "%lu", rtc_getUnixtime());
	return coap_make_response(scratch, outpkt, (const uint8_t *)&buffer[0], len, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static int handle_put_clock(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	if (inpkt->payload.len > 0) {
		rtc_setUnixtime(atoi_long(&inpkt->payload.p[0], inpkt->payload.len));
		// Antwort erzeugen:
		char buffer[11]; // Max. Länge 10 Zeichen
		unsigned char len;
		len = snprintf(&buffer[0], sizeof(buffer), "%lu", rtc_getUnixtime());
		return coap_make_response(scratch, outpkt, (const uint8_t *)&buffer[0], len, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CHANGED, COAP_CONTENTTYPE_TEXT_PLAIN);
	}
	return coap_make_response(scratch, outpkt, NULL, 0, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_BAD_REQUEST, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_unlock_barrier = {1, {"unlock"}};
static int handle_put_unlock_barrier(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	if (inpkt->payload.len > 0) {
		if (ctrl_isSpaceAvailable()) {
			unsigned long int ID = atoi_long(&inpkt->payload.p[0], inpkt->payload.len);
			if (ctrl_unlockBarrierFor(ID)) {
				return coap_make_response(scratch, outpkt, (const uint8_t *)"1", 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CHANGED, COAP_CONTENTTYPE_TEXT_PLAIN);
			}
		}
		// Antwort erzeugen:
		return coap_make_response(scratch, outpkt, (const uint8_t *)"0", 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
	}
	return coap_make_response(scratch, outpkt, NULL, 0, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_BAD_REQUEST, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_parkingID = {1, {"parkingID"}};
static int handle_get_parkingID(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	char buffer[11];
	unsigned char len;
	len = snprintf(&buffer[0], sizeof(buffer), "%lu", accounting_getParkingID());
	return coap_make_response(scratch, outpkt, (const uint8_t *)&buffer[0], len, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_available = {1, {"available"}};
static int handle_get_available(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	return coap_make_response(scratch, outpkt, (const uint8_t *)(ctrl_isSpaceAvailable() ? "1" : "0"), 1, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_TEXT_PLAIN);
}

static const coap_endpoint_path_t path_parkingData = {1, {"parking_data"}};
static int handle_get_parkingData(coap_rw_buffer_t *scratch, const coap_packet_t *inpkt, coap_packet_t *outpkt, uint8_t id_hi, uint8_t id_lo, void * info) {
	unsigned char buffer[2000];
	unsigned short int len;
	len = accounting_exportParkingHistoryJSON(&buffer[0], sizeof(buffer));
	//if (len) {
		return coap_make_response(scratch, outpkt, (const uint8_t *)&buffer[0], len, id_hi, id_lo, &inpkt->tok, COAP_RSPCODE_CONTENT, COAP_CONTENTTYPE_APPLICATION_JSON);
	//} else {
	//	// Sollte nie passieren, der Puffer ist groß genug für > 20 Log-Einträge (wir behalten maximal 16 im Speicher)
	//}
}



const coap_endpoint_t endpoints[] = {
	{COAP_METHOD_GET, handle_get_well_known_core, &path_well_known_core, "ct=40"},
	{COAP_METHOD_GET, handle_get_light, &path_light, "ct=0"},
	{COAP_METHOD_PUT, handle_put_light, &path_light, NULL},
	{COAP_METHOD_GET, handle_get_barrier, &path_barrier, "ct=0"},
	{COAP_METHOD_GET, handle_get_cardetect, &path_cardetect, "ct=0"},
	{COAP_METHOD_GET, handle_get_sequence, &path_sequence, "ct=0"},
	{COAP_METHOD_GET, handle_get_clock, &path_clock, "ct=0"},
	{COAP_METHOD_PUT, handle_put_clock, &path_clock, "ct=0"},
	{COAP_METHOD_PUT, handle_put_unlock_barrier, &path_unlock_barrier, NULL},
	{COAP_METHOD_GET, handle_get_parkingID, &path_parkingID, "ct=0"},
	{COAP_METHOD_GET, handle_get_available, &path_available, "ct=0"},
	{COAP_METHOD_GET, handle_get_parkingData, &path_parkingData, "ct=50"}, // 50 = Content-Type: JSON
	{(coap_method_t)0, NULL, NULL, NULL}
};













// Aufbau des .well-known-Endpunkts / Discovery:
// TODO: Ist ja schön dass das automatisch geht, es braucht aber aufgrund der unklaren Größe dauerhaft saumäßig viel RAM. Außerdem prüft
// die Funktion nicht auf einen Überlauf - zu viele Endpunkte würden also alles kaputt machen. Da die hier statisch implementiert sind
// ist das so erstmal in Ordnung aber nicht sehr schön.
void build_rsp(void) {
	uint16_t len = rsplen;
	const coap_endpoint_t *ep = endpoints;
	int i;
	
	len--; // Null-terminated string
	
	while (NULL != ep->handler) {
		if (NULL == ep->core_attr) {
			ep++;
			continue;
		}
		
		if (0 < strlen(rsp)) {
			strncat(rsp, ",", len);
			len--;
		}
		
		strncat(rsp, "<", len);
		len--;
		
		for (i=0; i<ep->path->count; i++) {
			strncat(rsp, "/", len);
			len--;
			strncat(rsp, ep->path->elems[i], len);
			len -= strlen(ep->path->elems[i]);
		}
		
		strncat(rsp, ">;", len);
		len -= 2;
		
		strncat(rsp, ep->core_attr, len);
		len -= strlen(ep->core_attr);
		
		ep++;
	}
}









// Copy of the main_posix.c file provided with microcoap as an example:

/**
*	\brief Processes message recognized by layer 5 (communication.c) for CoAP layer
*/
void ce_processIncomingMessage(MESSAGE_t * msg) {
	// Function is not reentrant (buffers reside in external memory since we only have 8 KB internal SRAM):
	static unsigned char buf[4096] EXTRAM;
	static unsigned char scratch_raw[4096] EXTRAM;
	coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
	coap_packet_t pkt;
	
	// Hand this message to CoAP for processing and transmit result back to sender
	
	// Nachricht raus kopieren (wäre vermutlich nicht nötig, bevor wir aber die komplette Bibliothek analysieren ob das überschrieben wird ist das so besser):
	memcpy(buf, msg->content, msg->len);
	
	if (!coap_parse(&pkt, buf, msg->len)) {
		
		// Nachricht erfolgreich geparsed
		coap_packet_t rsppkt;
		size_t rsplen = sizeof(buf);
		
		// Request verarbeiten, Antwort geht in buffer/rsppkt
		coap_handle_req(&scratch_buf, &pkt, &rsppkt, msg);
		
		if (!coap_build(&buf[0], &rsplen, &rsppkt)) {
			if (rsplen <= (MAX_PAYLOAD_LENGTH)) {
				// Neue Nachricht daraus bauen und abschicken:
				MESSAGE_t rspMsg;
				memset(&rspMsg, 0, sizeof(MESSAGE_t));
				// Antwort an den Absender des Request:
				rspMsg.msgDest = msg->msgSrc;
				// Ports mit dazu (für eventuell nötiges Routing):
				rspMsg.srcPort = msg->dstPort;
				rspMsg.dstPort = msg->srcPort;
				// Wir verschlüsseln wenn der Request ebenfalls verschlüsselt war:
				if (msg->flags & MSG_FLAG_ENCRYPTED)
					rspMsg.flags = MSG_FLAG_ENCRYPTED;
				// CoAP-Antwort (subProtocol wird implizit auf 0 gelassen):
				rspMsg.subProtocol = 0;
				rspMsg.sequence = &settings.maxKnownSequence[0];
				// Content übernehmen wir von der coap_build-Funktion:
				rspMsg.content = &buf[0];
				rspMsg.len = (unsigned short int)rsplen;
				
				// Und raus damit..
				com_sendProtocolMessage(uartPC, &rspMsg);
			} else {
				// Problem, CoAP-Nachricht ist eigentlich zu lang!
				ON(LED2);
			}
		} else {
			// CoAP build failed
			uart_printf_P(PSTR("CoAP build failed!"));
			ON(LED1);
		}
	} else {
		// Invalid CoAP message. Ignore.
	}
}


