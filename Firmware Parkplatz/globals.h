#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "aes.h"

/**
*	\brief Anything of global interest
*/
typedef struct {
	unsigned char carDetected;						/**< Has a car been detected? */
	unsigned char skipBarrierCollisionDetection;	/**< Are we skipping safety checks while raising the barrier? */
} GLOBALS_t;

extern GLOBALS_t globals;



#endif
