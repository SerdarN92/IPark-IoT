#ifndef _RTC_H_
#define _RTC_H_

/**
*	\brief Start hardware realtime clock
*/
void rtc_init();

/**
*	\brief Set hardware RTC to time
*	@param unixtime Timestamp to set to
*/
void rtc_setUnixtime(unsigned long int unixtime);

/**
*	\brief Return current timestamp from RTC
*	@returns Timestamp
*/
unsigned long int rtc_getUnixtime();

#endif
