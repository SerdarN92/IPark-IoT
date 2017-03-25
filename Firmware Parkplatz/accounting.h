#ifndef _ACCOUNTING_H_
#define _ACCOUNTING_H_

/**
* \brief Start parking of a car.
* @param ID The ID given by the controller for the parking event.
*/
void accounting_startParking(unsigned long int ID);

/**
* \brief Stop the ongoing parking event
*/
void accounting_stopParking();

/**
* \brief Set the system clock to another value, adjust all parking events in the history
* @param oldTime Old system time
* @param newTime New system time
*/
void accounting_setNewClock(unsigned long int oldTime, unsigned long int newTime);

/**
* \brief Query if there is an ongoing parking event
* @returns 1 if a parking event is commencing, 0 otherwise
*/
unsigned char accounting_isCounting();

/**
* \brief Get the controller's ID for the ongoing parking event
* @returns ID of the parking event
*/
unsigned long int accounting_getParkingID();

/**
* \brief Write parking history into buffer in JSON format
* @param buffer Pointer to a char buffer
* @param buflen Available length of the buffer in bytes
* @returns Length of written bytes
*/
unsigned short int accounting_exportParkingHistoryJSON(unsigned char * buffer, unsigned short int buflen);

#endif
