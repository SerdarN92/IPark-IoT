#include "control.h"
#include "common.h"
#include "servo.h"
#include "console.h"
#include "distance.h"
#include "systimer.h"
#include "hardware.h"
#include "light.h"
#include "accounting.h"
#include "globals.h"

#define NUMBER_OF_SENSORS 1

enum {
	CSTATE_BARRIER_GO_UP,
	CSTATE_INIT_CHECK_FREE,
	CSTATE_INIT_RAISE_BARRIER,
	CSTATE_BARRIER_IS_UP,
	CSTATE_BARRIER_GOING_DOWN,
	CSTATE_BARRIER_IS_DOWN,
	CSTATE_CHECK_CAR_PARKING,
	CSTATE_CAR_PARKING,
};

/**
*	\brief Structure to save all information regarding parking events
*/
typedef struct {
	unsigned char carParked[NUMBER_OF_SENSORS];	/**< What the sensor says about a parked car */
	unsigned char state;						/**< Internal state for state machine */
	unsigned char barrierEngaged;				/**< Is the barrier currently engaged? */
	unsigned short int delay;					/**< Delay counter for state machine */
	unsigned short int counter;					/**< Parking counter */
	unsigned char wantedState;					/**< Wanted state */
	unsigned char barrierIsMoving;				/**< Is the barrier moving right now? */
	unsigned char readyToUnlock;				/**< Is the parking space ready to be unlocked */
	unsigned long int unlockForID;				/**< Unlock for what ID */
	unsigned short int detectDistance;			/**< Detection distance */
} CONTROL_t;
CONTROL_t ctrl;

#define DISTANCE_CAR_DETECT			200
#define DISTANCE_BARRIER_DETECT		100

#define BARRIER_UP_FLASH_TIME_SEC					10	// 10 Sekunden blinkt die Ampel, dann geht die Sperre hoch wenn kein Fahrzeug erkannt wurde
#define BARRIER_DOWN_CANCEL_PARKING_DELAY_SEC		60	// 60 Sekunden hat der Autofahrer, auf den Parkplatz zu fahren (f¸r Demo so niedrig!)
#define CAR_PARKED_TO_COUTING_DELAY_SEC				10	// 10 Sekunden wartet der Parkplatz und testet kontinuierlich ob ein Auto da ist bis die Parkzeit gez‰hlt wird (f¸r Demo so niedrig!)
#define COUNTING_TO_NO_CAR_PARKED_DELAY_SEC			10	// 10 Sekunden wartet der Parkplatz und testet auf frei sein bevor nach einem Parken die Ampel anf‰ngt zu blinken (f¸r Demo so niedrig!)



void control_work(void * p);
void control_workBarrier(void * p);

void control_init(void) {
	timer_addCallback(1, 1, control_work, 0);
	timer_addCallback(1, 1, control_workBarrier, 0);
}

void control_work(void * p) {
	static unsigned char running = 0;
	
	if (ctrl_isNoCarParked()) {
		ON(LED4);
	} else {
		OFF(LED4);
	}
	if (ctrl_isCarParked()) {
		ON(LED5);
	} else {
		OFF(LED5);
	}
	
	
	if (running)
		return;
	running = 1;
	
	// If we need to delay the next step that's done here:
	if (ctrl.delay) {
		ctrl.delay--;
		running = 0;
		return;
	}
	
	switch (ctrl.state) {
		case CSTATE_BARRIER_GO_UP:
			// Gerade erst eingeschaltet worden. Der Servo muss im Grundzustand auf "Parkplatz frei" stehen damit wir ein potentiell parkendes Auto nicht besch‰digen kˆnnen.
			ctrl_disengageBarrier();
			
			// Parkplatz optisch "sperren" (mit blinken!)
			light_setMode(LIGHT_FLASH_RED);
			
			// Einige Zeit warten bis wir den Parkplatz auf Freisein testen
			ctrl.delay = 2500; // Festes Delay um genug Sensordaten zu sammeln
			
			// Als n‰chstes auf Freisein testen und Sperre hoch fahren:
			ctrl.state = CSTATE_INIT_CHECK_FREE;
			
			ctrl.counter = 0; // Neu z‰hlen
			break;
			
		case CSTATE_INIT_CHECK_FREE:
			if (ctrl_isNoCarParked()) {
				ctrl.counter++;
				if (ctrl.counter > BARRIER_UP_FLASH_TIME_SEC) { // ‹ber eine Sekunde testen
					uart_printf_P(PSTR("Sperre hoch..\n"));
					// Sperre jetzt hoch fahren:
					ctrl_engageBarrier();
					// Erkennungsdistanz unempfindlicher, Sperre geht hoch.
					//ctrl.detectDistance = DISTANCE_CAR_DETECT;
					ctrl.detectDistance = DISTANCE_BARRIER_DETECT;
					
					ctrl.counter = 0; // Neu z‰hlen
					ctrl.state = CSTATE_INIT_RAISE_BARRIER;
					break;
				}
			} else {
				// Parkplatz muss AM ST‹CK frei sein
				ctrl.counter = 0;
			}
			ctrl.delay = 1000; // Jede Sekunde testen
			break;
			
		case CSTATE_INIT_RAISE_BARRIER:
			ctrl.delay = 10;
			if (globals.skipBarrierCollisionDetection || ctrl_isNoCarParked()) { // Sobald wir nicht mehr sicher sind m¸ssen wir die Sperre senken und zur¸ck gehen..
				ctrl.counter++;
				if (!ctrl.barrierIsMoving) { // 10ms * 100 = 1s
					ctrl.state = CSTATE_BARRIER_IS_UP;
					uart_printf_P(PSTR("Sperre ist HOCH.\n"));
					// Ab jetzt kann man ¸berhaupt erst den Parkplatz f¸r jemand freigeben
					ctrl.unlockForID = 0;
					ctrl.readyToUnlock = 1;
				}
			} else {
				ctrl_disengageBarrier(); // Sperre wieder runter, sicherheitshalber
				uart_printf_P(PSTR("Fahrzeug im Weg! Senke Sperre.\n"));
				ctrl.state = CSTATE_BARRIER_GO_UP;
			}
			break;
			
		// IDLE-Zustand:
		case CSTATE_BARRIER_IS_UP:
			if (/*sperre runter machen */ ISON(KEY0) || ctrl.unlockForID) {
				ctrl.readyToUnlock = 0;
				uart_printf_P(PSTR("Parkplatz angefordert.\n"));
				// Sperre runter fahren:
				light_setMode(LIGHT_FLASH_RED);
				ctrl_disengageBarrier();
				ctrl.delay = 2500;
				ctrl.state = CSTATE_BARRIER_GOING_DOWN;
				break;
			}
			ctrl.delay = 100;
			light_setMode(LIGHT_RED); // Rotes Dauerlicht
			break;
			
		case CSTATE_BARRIER_GOING_DOWN:
			if (ctrl.barrierIsMoving) // Warten bis die Sperre runter ist
				break;
			light_setMode(LIGHT_GREEN);
			uart_printf_P(PSTR("Parkplatz ist befahrbar.\n"));
			ctrl.counter = 0;
			ctrl.state = CSTATE_BARRIER_IS_DOWN;
			// Erkennungsdistanz empfindlicher, Sperre ist runter.
			ctrl.detectDistance = DISTANCE_CAR_DETECT;
			//ctrl.detectDistance = DISTANCE_BARRIER_DETECT;
			
			break;
			
		case CSTATE_BARRIER_IS_DOWN:
			// Jetzt warten wir bis zu zwei Minute darauf, das ein Auto auf den Parkplatz f‰hrt
			if (ctrl_isCarParked()) {
				light_setMode(LIGHT_GREEN);
				ctrl.counter = 0;
				ctrl.state = CSTATE_CHECK_CAR_PARKING;
			}
			ctrl.counter++;
			// 10 Sekunden bevor wir die Sperre wieder aktivieren muss die Ampel wieder auf rot.
			if (ctrl.counter == (BARRIER_DOWN_CANCEL_PARKING_DELAY_SEC - BARRIER_UP_FLASH_TIME_SEC)) {
				light_setMode(LIGHT_FLASH_RED);
			} else if (ctrl.counter > BARRIER_DOWN_CANCEL_PARKING_DELAY_SEC) {
				uart_printf_P(PSTR("Kein Fahrzeug geparkt - sperre Parkplatz..\n"));
				// Parkplatz wieder sperren, es f‰hrt kein Kunde drauf..!
				ctrl.state = CSTATE_BARRIER_GO_UP;
				ctrl.counter = 0;
			}
			ctrl.delay = 1000; // Sekunden-Ticks
			break;
			
		case CSTATE_CHECK_CAR_PARKING:
			if (ctrl.counter < CAR_PARKED_TO_COUTING_DELAY_SEC) {
				if (ctrl_isCarParked()) {
					ctrl.counter++;
				} else {
					uart_printf_P(PSTR("Doch kein Auto..?\n"));
					ctrl.state = CSTATE_BARRIER_IS_DOWN;
				}
			} else {
				uart_printf_P(PSTR("Auto geparkt.\n"));
				
				// Accounting starten mit der angegebenen Reservierungsnummer:
				accounting_startParking(ctrl.unlockForID);
				
				// Okay, Light auf rot, Auto ist offenbar geparkt
				light_setMode(LIGHT_RED); // Rotes Dauerlicht, Fahrzeug erkannt. TODO: Anderes Lichtzeichen so dass der Nutzer erkennt das abgerechnet wird? Licht ganz aus?
				
				ctrl.counter = 0;
				ctrl.state = CSTATE_CAR_PARKING;
			}
			ctrl.delay = 1000; // Sekunden-Ticks
			break;
			
		case CSTATE_CAR_PARKING:
			if (ctrl_isNoCarParked()) {
				if (ctrl.counter > COUNTING_TO_NO_CAR_PARKED_DELAY_SEC) {
					light_setMode(LIGHT_FLASH_RED); // Sperre geht gleich hoch
					uart_printf_P(PSTR("Auto verl‰sst Parkplatz..\n"));
					ctrl.state = CSTATE_BARRIER_GO_UP;
					// Abrechnung hier abschlieﬂen:
					accounting_stopParking();
				} else {
					ctrl.counter++;
				}
			} else {
				ctrl.counter = 0;
			}
			ctrl.delay = 1000; // Sekunden-Takt
			break;
	}

	running = 0;
}

void control_workBarrier(void * p) {
	static unsigned short int barrierPosition;
	if (ctrl.barrierEngaged) {
		if (barrierPosition < 4500 /*1200*/) {
			barrierPosition++;
			ctrl.barrierIsMoving = 1;
		} else {
			ctrl.barrierIsMoving = 0;
		}
	} else {
		if (barrierPosition) {
			barrierPosition--;
			ctrl.barrierIsMoving = 1;
		} else {
			ctrl.barrierIsMoving = 0;
		}
	}
	servo_setServoPosition(&TCC0, 0, barrierPosition);
}

unsigned char ctrl_isBarrierEngaged() {
	return ctrl.barrierEngaged;
}

unsigned char ctrl_isCarParked() {
	unsigned char i = NUMBER_OF_SENSORS;
	unsigned char decision = 128;
	while (i--) {
		if (ctrl.carParked[i] > 15) {
			decision++;
		} else if (ctrl.carParked[i] < 5) {
			decision--;
		}
	}
	if (decision > 128)
		return 1;
	return 0;
}

unsigned char ctrl_isNoCarParked() {
	unsigned char i = NUMBER_OF_SENSORS;
	unsigned char decision = 128;
	while (i--) {
		if (ctrl.carParked[i] > 15) {
			decision++;
		} else if (ctrl.carParked[i] < 5) {
			decision--;
		}
	}
	if (decision < 128)
		return 1;
	return 0;
}

void ctrl_engageBarrier() {
	ctrl.barrierEngaged = 1;
	ctrl.barrierIsMoving = 1;
}

void ctrl_disengageBarrier() {
	ctrl.barrierEngaged = 0;
	ctrl.barrierIsMoving = 1;
}

void ctrl_sensorValue(unsigned char sensorNo, unsigned short int distance) {
	if (!(sensorNo < NUMBER_OF_SENSORS))
		return;
	unsigned char * cp = &ctrl.carParked[sensorNo];
	// carPresent > 15: JA
	// carPresent < 5: Nein
	// Sonst: (Nicht sicher)
	if (distance < 5 || distance > 0xff00) {
		if (*cp > 10) {
			(*cp)--;
		} else if (*cp < 10) {
			(*cp)++;
		}
		// Ung¸ltige Messung. Kam hier im Wechsel mit g¸ltigen Messungen mit genauen Messwerten vor, eventuell muss man das ignorieren und nur die g¸ltigen Messungen verwenden..
	} else {
		if (distance > ctrl.detectDistance) {
			if (*cp)
				(*cp)--;
		} else {
			if (*cp < 20)
				(*cp)++;
		}
	}
	if (*cp < 5)
		OFF(LED7);
	if (*cp > 15)
		ON(LED7);
	if (ISON(KEY1))
		uart_printf_P(PSTR("Dist %u\n"), distance);
}

unsigned char ctrl_isSpaceAvailable() {
	if (ctrl.readyToUnlock && !ctrl.unlockForID) {
		return 1;
	}
	return 0;
}

unsigned char ctrl_unlockBarrierFor(unsigned long int ID) {
	if (ctrl.readyToUnlock) {
		ctrl.unlockForID = ID;
		ctrl.readyToUnlock = 0;
		return 1;
	}
	return 0;
}
