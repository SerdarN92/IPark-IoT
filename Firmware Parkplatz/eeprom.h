#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "callbackDefs.h"

/**
*	\brief Initialize EEPROM
*/
void ee_init();

/**
*	\brief Save data to EEPROM
*	@param[in] eeprom_address Address to save to
*	@param[in] data Pointer to data
*	@param[in] dataLen Length in bytes
*	@param[in] wait Do wait for write to finish
*	@returns 1 if successfull/write has been started, 0 otherwise
*/
unsigned char ee_saveData(unsigned int eeprom_address, void* data, unsigned int dataLen, unsigned char wait);

/**
*	\brief Save data to EEPROM, saving life cycles of the EEPROM
*	@param[in] eeprom_address Address to save to
*	@param[in] data Pointer to data
*	@param[in] dataLen Length in bytes
*	@param[in] wait Do wait for write to finish
*	@returns 1 if successfull/write has been started, 0 otherwise
*/
unsigned char ee_saveDataEx(unsigned int eeprom_address, void* data, unsigned int dataLen, unsigned char wait);

/**
*	\brief Load data from EEPROM
*	@param[in] eeprom_address Address to load from
*	@param[out] data Pointer where data should be written to
*	@param[in] dataLen Length in bytes
*/
void ee_loadData(unsigned int eeprom_address, void* data, unsigned int dataLen);

/**
*	\brief Return if EEPROM is busy
*	@returns 1 if busy, else 0
*/
unsigned char ee_isBusy(void);


void ee_asyncWrite(unsigned int eeprom_address, void* data, unsigned int dataLen);
/**
*	\brief Start asynchronous write to EEPROM
*	@param[in] eeprom_address Address to save to
*	@param[in] data Pointer to data
*	@param[in] dataLen Length in bytes
*	@param[in] callback Function to be called when write has finished
*/
void ee_asyncWriteEx(unsigned int eeprom_address, void* data, unsigned int dataLen, Void_Function_Void callback);




#endif
