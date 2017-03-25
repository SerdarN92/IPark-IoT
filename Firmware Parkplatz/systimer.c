#include <avr/interrupt.h>
#include "common.h"
#include <avr/io.h>
#include "systimer.h"
#include "hardware.h"

#ifndef F_TIMER
#define F_TIMER		F_CPU
#endif

#define SYSTIMER_PRESCALER0			256UL // Prescaler wird auf 256 initialisiert
#define SYSTIMER_RESOLUTION0		5UL   // 5 Teilschritte (Jeder Teilschritt mit 200 Hz => 1000 Hz / 5)
#define SYSTIMER_FREQ_ONE_ROUND0	200UL // 1000 Hz = 1ms

// * 10 damit wir keine Böse überraschung erleben weil das ganze nicht glatt teilbar ist; -50 um die 5 zu späten Überläufe zu kompensieren
#define SYSTIMER_MULT0				(((F_TIMER * 10UL) / (SYSTIMER_PRESCALER0 * SYSTIMER_FREQ_ONE_ROUND0)) - (SYSTIMER_RESOLUTION0 * 10UL))
#define SYSTIMERVAL0EXT(x)			((((SYSTIMER_MULT0 / SYSTIMER_RESOLUTION0 * x) + 5UL) / 10UL) - (x ? (((SYSTIMER_MULT0 / SYSTIMER_RESOLUTION0 * (x - 1)) + 5UL) / 10UL) : 0))
#define SYSTIMERVAL0(x)				(unsigned char)SYSTIMERVAL0EXT(x)

#define EMERGENCY_TIMER_TICKS		10000

#define PRECISE_TICKS_PER_10MS		((unsigned long int)(F_CPU >> 8) / 100)

static unsigned long uptime = 0;
static unsigned long int startTime = 0;
static unsigned long int systimer_time = 0;
static volatile unsigned char systimer_toAdd = 0;
static unsigned char systimer_round = 0;
static unsigned char systimer_running = 0;
//static unsigned int emergencyTimer = EMERGENCY_TIMER_TICKS;
static unsigned long int preciseTime;

/**
*	\brief Structure to manage timed callbacks
*/
typedef struct tmr_callback {
	unsigned int waitTime;			/**< Time until callback is due */
	unsigned int restartWith;		/**< Restart with this time */
	Void_Function_VoidP callback;	/**< Function to call as callback */
	unsigned char * callback_ptr;	/**< Pointer is passed to callback */
	unsigned char isActive:1;		/**< Is callback currently active (recursive call)? */
} timer_callback_t;

// Struct ist 6 Byte groß, also hier 150 Byte RAM "verbraten":
#define TIMER_NR_OF_CALLBACKS		25
static timer_callback_t callbacks[TIMER_NR_OF_CALLBACKS]; // Zählen über STRG+D "timer_addCallback" - 2  ==>  24 bei der letzten Zählung

static unsigned char callback_count = 0;

#define TIMER				TCD0
#define TIMER_vect			TCD0_OVF_vect

void systimer_init() {
	TIMER.CTRLB = TC_WGMODE_NORMAL_gc;
	TIMER.INTCTRLA |= TC_OVFINTLVL_HI_gc;
	TIMER.PERL = SYSTIMERVAL0(1) & 0xff;
	TIMER.PERH = SYSTIMERVAL0(1) >> 8;
	TIMER.CTRLA = TC_CLKSEL_DIV256_gc;
	systimer_running = 1;
}

unsigned char systimer_isTimerRunning() {
	return systimer_running;
}

unsigned long int getRetAddr(unsigned int stackposition);

// Routine, um den Stackpointer zu ermitteln
unsigned long int getRetAddr(unsigned int stackposition) {
	return stackposition;
	
	//unsigned long int * retAddr = (unsigned long int*)(unsigned int)(stackposition);
	/*
	//unsigned char* stack = (unsigned char*)(unsigned int)stackposition;
	//unsigned long int retAddr = 0;
	
	//retAddr |= *stack;
	//stack++;
	//retAddr <<= 8;
	//retAddr |= *stack;
	//stack++;
	//retAddr <<= 8;
	//retAddr |= *stack;
	*/
	//return (*retAddr);
}

ISR(TIMER_vect) {
	//unsigned int sPointer = (unsigned int)(SPH << 8) | (SPL);
	{
		//unsigned long int position = 0xffffffff;
		unsigned char i = 0;
		
		switch (systimer_round++) {
			case 0:
				TIMER.PERBUFL = SYSTIMERVAL0(1) & 0xff;
				TIMER.PERBUFH = SYSTIMERVAL0(1) >> 8;
				// Vorherige Runde war 5
				preciseTime += SYSTIMERVAL0(5);
				break;
			case 1:
				TIMER.PERBUFL = SYSTIMERVAL0(2) & 0xff;
				TIMER.PERBUFH = SYSTIMERVAL0(2) >> 8;
				// Vorherige Runde war 1
				preciseTime += SYSTIMERVAL0(1);
				break;
			case 2:
				TIMER.PERBUFL = SYSTIMERVAL0(3) & 0xff;
				TIMER.PERBUFH = SYSTIMERVAL0(3) >> 8;
				// Vorherige Runde war 2
				preciseTime += SYSTIMERVAL0(2);
				break;
			case 3:
				TIMER.PERBUFL = SYSTIMERVAL0(4) & 0xff;
				TIMER.PERBUFH = SYSTIMERVAL0(4) >> 8;
				// Vorherige Runde war 3
				preciseTime += SYSTIMERVAL0(3);
				break;
			case 4:
				TIMER.PERBUFL = SYSTIMERVAL0(5) & 0xff;
				TIMER.PERBUFH = SYSTIMERVAL0(5) >> 8;
				// Vorherige Runde war 4
				preciseTime += SYSTIMERVAL0(4);
				systimer_round = 0;
				break;
		}
		
		systimer_toAdd++;
		if (!systimer_toAdd) {
			// Overflow des Systimers. Hier MÜSSEN wir die systimer_time erhöhen, sonst vergessen wir 256ms!
			systimer_time+=0x100;
			/*
			if (ISON(LED_EIN)) {
				OFF(LED_EIN);
			} else {
				ON(LED_EIN);
			}
			*/
		}
		/*
		if (ISON(KEY_ON)) {
			if (!emergencyTimer) {
				position = getRetAddr(sPointer + 0x15);
				OFF(MODUL_ON);
				OFF(LED_EIN);
				emergencyTimer = EMERGENCY_TIMER_TICKS >> 1;
				if (device_emergencyTurnOff(position))
					while (1);
			} else {
				emergencyTimer--;
			}
		} else {
			emergencyTimer = EMERGENCY_TIMER_TICKS;
		}
		*/
		for (i=0;i<callback_count;i++) {
			if (callbacks[i].waitTime)
				callbacks[i].waitTime--;
		}
	}
}

unsigned long systimer_getUptime() {
	return uptime;
}

unsigned int check_timers() {
	unsigned char toAdd = 0;
	unsigned char i = 0;
	unsigned char stmp;
	void * ptr;
	static unsigned char callRound = 0;
	Void_Function_VoidP thisCallback;
	
	if (callRound > 10)
		return 0;
	
	callRound++;
	
	if (systimer_toAdd) {
		stmp = SREG;
		cli();
		toAdd = systimer_toAdd;
		systimer_toAdd = 0;
		systimer_time+=toAdd;
		SREG = stmp;
	} else {
		callRound--;
		return 0;
	}
	
	uptime += toAdd;
	
	
	
	// We only have to check this if a 1ms tick has happened.
	//timer_msExpired(toAdd);
	
	i = callback_count;
	while (i--) {
		if (!callbacks[i].waitTime && !callbacks[i].isActive) {
			thisCallback = callbacks[i].callback;
			ptr = callbacks[i].callback_ptr;
		
			if (callbacks[i].restartWith) {
				callbacks[i].waitTime = callbacks[i].restartWith;
			} else {
				callbacks[i] = callbacks[callback_count--];
			}
		
			// Dieser Callback ist gerade aktiv:
			callbacks[i].isActive = 1;
			// Callback aufrufen:
			thisCallback(callbacks[i].callback_ptr);
			// Wenn dieser Callback gerade evtl gelöscht wurde, dürfen wir isActive nicht löschen:
			if ((callbacks[i].callback == thisCallback) && (callbacks[i].callback_ptr == ptr)) {
				callbacks[i].isActive = 0;
			} else {
				// Irgendwo ist jetzt ein isActive-Bit gesetzt wo es das nicht sein sollte. Das müssen wir überprüfen.
				unsigned char n = callback_count;
				while (n--) {
					if ((callbacks[n].callback == thisCallback) && (callbacks[n].callback_ptr == ptr) && callbacks[n].isActive) {
						callbacks[n].isActive = 0;
						break;
					}
				}
			}
		}
	}
	
	callRound--;
	
	return toAdd;
}

unsigned char timer_getCallbackCount() {
	return callback_count;
}

void timer_addCallback(unsigned int waitTime, unsigned char repeat, Void_Function_VoidP fnct, void * ptr) {
	unsigned char i = 0;
	while (i < callback_count) {
		if ((callbacks[i].callback == fnct) && (callbacks[i].callback_ptr == ptr)) {
			break;
		}
		i++;
	}
	if (i < TIMER_NR_OF_CALLBACKS) {
		callbacks[i].waitTime = waitTime;
		callbacks[i].restartWith = (repeat ? waitTime : 0);
		callbacks[i].callback = fnct;
		callbacks[i].isActive = 0;
		callbacks[i].callback_ptr = (unsigned char*)ptr;
		if (callback_count == i) {
			callback_count++;
		}
	}
}

void timer_delCallback(Void_Function_VoidP fnct, void * ptr) {
	unsigned char i = callback_count;
	if (!fnct) {
		callback_count = 0;
		return;
	}
	while (i--) {
		if ((callbacks[i].callback == fnct) && (callbacks[i].callback_ptr == ptr)) {
			callback_count--;
			callbacks[i] = callbacks[callback_count];
			break;
		}
	}
}

void systimer_startStopwatch() {
	unsigned char stmp;
	check_timers();
	stmp = SREG;
	cli();
	startTime = systimer_time;
	SREG = stmp;
}

unsigned long int systimer_getStopwatch() {
	unsigned long int tmp;
	unsigned char stmp;
	check_timers();
	stmp = SREG;
	cli();
	tmp = (systimer_time - startTime);
	SREG = stmp;
	return tmp; 
}

unsigned long int systimer_getTimestamp_async() {
	return systimer_time + systimer_toAdd;
}

unsigned long int systimer_getTimestamp() {
	unsigned long int tmp;
	unsigned char stmp;
	stmp = SREG;
	cli();
	tmp = systimer_time + systimer_toAdd;
	SREG = stmp;
	return tmp;
}

unsigned long int timer_getPreciseTimestamp() {
	unsigned char timerValueL;
	unsigned char timerValueH;
	unsigned long int preciseTime_local;
	cli();
	timerValueL = TIMER.CNTL;
	timerValueH = TIMER.CNTH;
	preciseTime_local = preciseTime;
	sei();
	// Okay. preciseTime_local enthält jetzt die Zeit die aktuell war, als der Timer noch vor einem möglichen Überlauf stand.
	return preciseTime_local + (((unsigned int)timerValueH << 8) | timerValueL);
}

unsigned long int timer_getPreciseTimestamp_async() { // Wir sind sowieso in einem Interrupt, können also eh nicht unterbrochen werden.
	unsigned char timerValueL;
	unsigned char timerValueH;
	unsigned long int preciseTime_local;
	timerValueL = TIMER.CNTL;
	timerValueH = TIMER.CNTH;
	preciseTime_local = preciseTime;
	// Okay. preciseTime_local enthält jetzt die Zeit die aktuell war, als der Timer noch vor einem möglichen Überlauf stand.
	return preciseTime_local + (((unsigned int)timerValueH << 8) | timerValueL);
}

unsigned long int timer_getMicrosecondsFromPreciseTimestamp(unsigned long int preciseTimestamp) {
	return (preciseTimestamp * 10000) / PRECISE_TICKS_PER_10MS;
}
