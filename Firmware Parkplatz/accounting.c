#include "accounting.h"
#include "console.h"
#include "control.h"
#include "rtc.h"
#include <stdio.h>
#include <string.h>

#define PARKING_HISTORY_ITEMS	16	/** How many items are kept in the parking history */

/**
*	\brief Keeps information regarding a parking event.
*/
typedef struct { 
	unsigned long int startTime; /**< Start time of the parking event in local time (seconds). */
	unsigned long int stopTime; /**< End time of the parking event in local time (seconds). */
	unsigned long int ID; /**< ID provided by the controller when lowering the barrier for parking. */
} PARK_HISTORY_t;

static PARK_HISTORY_t parkHistory[PARKING_HISTORY_ITEMS]; /**< Parking event history */
static unsigned char isParking; /**< Is a car parking right now? */
static unsigned long int parkingID; /**< Under what ID has the barrier been lowered for the current parking event */
static unsigned long int parkStartTime; /**< At what time was the currently running parking event startet? */
static unsigned char parkingHistoryCount; /**< How many valid items are stored in the parking history */


void accounting_logParkTime(unsigned long int startTime, unsigned long int stopTime, unsigned long int ID);

/**
* \brief Start parking of a car.
* @param ID The ID given by the controller for the parking event.
*/
void accounting_startParking(unsigned long int ID) {
	// Wer auch immer da auf den Parkplatz "gerutscht" ist - der vorherige Kunde ist offenbar weg:
	if (isParking)
		accounting_stopParking();
	isParking = 1;
	parkingID = ID;
	parkStartTime = rtc_getUnixtime();
}

/**
* \brief Stop the ongoing parking event
*/
void accounting_stopParking() {
	// Das kann nicht sein falls niemand parkt!
	if (!isParking)
		return;
	accounting_logParkTime(parkStartTime, rtc_getUnixtime(), parkingID);
	isParking = 0;
	parkingID = 0;
}

/**
* \brief Query if there is an ongoing parking event
* @returns 1 if a parking event is commencing, 0 otherwise
*/
unsigned char accounting_isCounting() {
	return isParking;
}

/**
* \brief Get the controller's ID for the ongoing parking event
* @returns ID of the parking event
*/
unsigned long int accounting_getParkingID() {
	return (isParking ? parkingID : 0);
}

/**
* \brief Set the system clock to another value, adjust all parking events in the history
* @param oldTime Old system time
* @param newTime New system time
*/
void accounting_setNewClock(unsigned long int oldTime, unsigned long int newTime) {
	unsigned char i;
	// Aufgezeichnete Parkzeiten an die neue Systemzeit anpassen (alle Timestamps etc müssen angepasst werden):
	if (isParking) {
		parkStartTime =- oldTime;
		parkStartTime += newTime;
	}
	for (i=0;i<parkingHistoryCount;i++) {
		parkHistory[i].startTime = parkHistory[i].startTime - oldTime + newTime;
		parkHistory[i].stopTime = parkHistory[i].stopTime - oldTime + newTime;
	}
}

/**
* \brief Add parking event to history
* @param startTime Starting time
* @param stopTime End time
* @param ID Controller's ID for the parking event
*/

void accounting_logParkTime(unsigned long int startTime, unsigned long int stopTime, unsigned long int ID) {
	unsigned char i;
	PARK_HISTORY_t * history = &parkHistory[0];
	// Alle alten Parkdaten verschieben (quick & dirty), ältesten Eintrag überschreiben:
	i = PARKING_HISTORY_ITEMS-1;
	while (i--)
		parkHistory[i+1] = parkHistory[i];
	history->startTime = startTime;
	history->stopTime = stopTime;
	history->ID = ID;
	uart_printf_P(PSTR("Parkzeit von %lu bis %lu fuer ID %lu."), startTime, stopTime, ID);
	// Count how many valid items we have in the history:
	if (parkingHistoryCount < PARKING_HISTORY_ITEMS)
		parkingHistoryCount++;
}

/**
* \brief Write parking history into buffer in JSON format
* @param buffer Pointer to a char buffer
* @param buflen Available length of the buffer in bytes
* @returns Length of written bytes
*/
unsigned short int accounting_exportParkingHistoryJSON(unsigned char * buffer, unsigned short int buflen) {
	unsigned short int len;
	unsigned short int msglen = 0;
	unsigned char i;
	
	if (buflen > 2) {
		buffer[msglen++] = '{';
		buflen--;
		
		if (ctrl_isSpaceAvailable()) {
			// Parkplatz verfügbar
			len = snprintf((char*)&buffer[msglen], buflen, "\"available\": 1, \"parking_duration\": 0, \"parking_since\": 0, \"ev\":[");
		} else if (isParking) {
			// Parkplatz nicht verfügbar sondern belegt:
			len = snprintf((char*)&buffer[msglen], buflen, "\"available\": 0, \"parking_duration\": %lu, \"parking_since\": %lu, \"ev\":[", rtc_getUnixtime() - parkStartTime, parkStartTime);
		} else {
			// Parkplatz weder verfügbar noch belegt, irgendein Fehlerzustand:
			len = snprintf((char*)&buffer[msglen], buflen, "\"available\": 0, \"parking_duration\": 0, \"parking_since\": 0, \"ev\":[");
		}

		if (len >= buflen) // Puffer zu knapp bemessen
			return 0;
		
		msglen += len;
		buflen -= len;
		
		for (i=0;i<parkingHistoryCount;i++) {
			if (i) {
				// Einzelne Objekte durch Komma trennen:
				buffer[msglen++] = ',';
				buflen--;
			}
			len = snprintf((char*)&buffer[msglen], buflen, "{\"index\": %u, \"startTime\": %lu, \"stopTime\": %lu, \"ID\": %lu}", i, parkHistory[i].startTime, parkHistory[i].stopTime, parkHistory[i].ID);
			
			if (len >= buflen) // Puffer zu knapp bemessen
				return 0;
			
			msglen += len;
			buflen -= len;
			
		}
		if (buflen > 1) {
			buffer[msglen++] = ']';
			buffer[msglen++] = '}';
			buflen-=2;
		}
	}
	return msglen;
}

