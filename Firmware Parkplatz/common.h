
#ifndef _COMMON_H_
#define _COMMON_H_

/**
* \brief Wait the specified number of milliseconds. Call other processes in the mean time if required.
* @param[in] milliseconds Number of milliseconds to wait
*/
void wait(unsigned short milliseconds);

/**
* \brief Wait the specified number of milliseconds NOT calling other processes.
* \sa wait
*/
void busy_wait(unsigned short milliseconds);

/**
* \brief Schedule waiting
*/
void start_wait(unsigned int howlong);
/**
* \brief Wait for the scheduled waiting time to expire
* @returns 1 if the time specified by @@start_wait has expired, 0 if not
*/
unsigned int do_wait();

/**
* \brief Reverse the byte order of the specified memory location
* @param[in,out] p Memory location to reverse byte order from
* @param[in] len Size of data type (1, 2, 4 bytes)
*/
void reverseByteOrder(void * p, unsigned char len);

/**
* \brief Convert a string to an unsigned long int (32 bit)
* @param[in] p Pointer to string
* @param[in] len Length of string
*/
unsigned long int atoi_long(const unsigned char * p, unsigned char len);

#endif
