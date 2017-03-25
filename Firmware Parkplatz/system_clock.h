#ifndef _LIB_SYSTEM_CLOCK_H_
#define _LIB_SYSTEM_CLOCK_H_

#define SYSTEM_CLOCK_2MHz	0
#define SYSTEM_CLOCK_32MHz	1

/**
*	\brief Set �C clock speed
*	@param clockMode	0: 2 MHz
*						1: 32 MHz
*/
void selectClockSource(unsigned char clockMode);












#endif
