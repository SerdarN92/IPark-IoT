#ifndef _DISTANCE_H_
#define _DISTANCE_H_

/**
*	\brief Initialize measurement of distance sensor
*/
void dist_init(void);

/**
*	\brief Read out last aquired distance
*/
unsigned int dist_getLastDistance();

#endif
