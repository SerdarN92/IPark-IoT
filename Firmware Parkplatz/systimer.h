#ifndef _SYSTIMER_H_
#define _SYSTIMER_H_

#include "callbackDefs.h"

void systimer_init();
unsigned int check_timers();

void timer_addCallback(unsigned int waitTime, unsigned char repeat, Void_Function_VoidP fnct, void * ptr);
void timer_delCallback(Void_Function_VoidP fnct, void * ptr);

void systimer_startStopwatch();
unsigned long int systimer_getStopwatch();
unsigned long int systimer_getTimestamp();
unsigned long int systimer_getTimestamp_async();

unsigned char systimer_isTimerRunning();

unsigned long int timer_getPreciseTimestamp();
unsigned long int timer_getPreciseTimestamp_async(); // Wir sind sowieso in einem Interrupt, können also eh nicht unterbrochen werden.

unsigned char device_emergencyTurnOff(unsigned long int interruptPosition);
void timer_msExpired(unsigned char msElapsed);

unsigned long systimer_getUptime();
unsigned char timer_getCallbackCount(void);

void timer_startOfCallback(void * proc);
void timer_endOfCallback(void * proc);

unsigned long int timer_getMicrosecondsFromPreciseTimestamp(unsigned long int preciseTimestamp);

#endif
