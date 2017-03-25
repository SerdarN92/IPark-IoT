#ifndef _SERVO_H_
#define _SERVO_H_

#include <avr/io.h>

/**
*	\brief Initialize servo driver
*	@param tc Pointer to Timer/Counter 0 to be used
*/
void servo_init(TC0_t * tc);

/**
*	\brief Set servo position
*	@param tc Timer/Counter 0 to be used
*	@param ccChannel Compare/Capture channel (depends on µC pin)
*	@param centiangle Angle to drive servo to, 400 steps for whole servo range
*/
void servo_setServoPosition(TC0_t * tc, unsigned char ccChannel, unsigned short int centiangle);


#endif
