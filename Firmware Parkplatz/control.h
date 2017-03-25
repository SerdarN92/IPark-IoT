#ifndef _CONTROL_H_
#define _CONTROL_H_

/**
*	\brief Initialize control functions
*/ 
void control_init();

/**
*	\brief Query if barrier is engaged
*	@returns 1 if barrier is engaged, 0 otherwise
*/
unsigned char ctrl_isBarrierEngaged();

/**
*	\brief Query if car is parked. Note: This is not exclusive to @@ctrl_isNoCarParked!
*	@returns 1 if car is parked, 0 otherwise
*/
unsigned char ctrl_isCarParked();

/**
*	\brief Query if NO car is parked. Note: This is not exclusive to @@ctrl_isCarParked!
*	@returns 1 if NO car is parked, 0 otherwise
*/
unsigned char ctrl_isNoCarParked();

/**
*	\brief Engage barrier
*/
void ctrl_engageBarrier();

/**
*	\brief Disengage barrier
*/
void ctrl_disengageBarrier();

/**
*	\brief Input sensor value
*	@param[in] sensorNo Valud for which sensor
*	@param[in] distance Distance measured by this sensor
*/
void ctrl_sensorValue(unsigned char sensorNo, unsigned short int distance);

/**
*	\brief Query if parking space is available
*	@returns 1 if space is available to be unlocked, 0 otherwise
*/
unsigned char ctrl_isSpaceAvailable();

/**
*	\brief Unlock the barrier for a car
*	@param ID ID given by the controller to identify this event
*/
unsigned char ctrl_unlockBarrierFor(unsigned long int ID);

#endif
