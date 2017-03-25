#include "hardware.h"
#include "light.h"
#include "systimer.h"

#define LIGHT_INTERVAL		10

static unsigned char light_mode;
static unsigned char lightCounter;
void light_work(void * p);

void light_init() {
	timer_addCallback(50, 1, light_work, 0);
	
}

void light_work(void * p) {
	if (lightCounter--)
		return;
	lightCounter = LIGHT_INTERVAL - 1;
	
	switch (light_mode) {
		case LIGHT_OFF:
			OFF(LIGHT_RED);
			OFF(LIGHT_GREEN);
			break;
		case LIGHT_RED:
			OFF(LIGHT_GREEN);
			ON(LIGHT_RED);
			break;
		case LIGHT_GREEN:
			ON(LIGHT_GREEN);
			OFF(LIGHT_RED);	
			break;
		case LIGHT_FLASH_RED:
			if (ISON(LIGHT_RED)) {
				OFF(LIGHT_RED);
			} else {
				ON(LIGHT_RED);
			}
			OFF(LIGHT_GREEN);
			break;
		case LIGHT_FLASH_GREEN:
			if (ISON(LIGHT_GREEN)) {
				OFF(LIGHT_GREEN);
			} else {
				ON(LIGHT_GREEN);
			}
			OFF(LIGHT_RED);
			break;
		case LIGHT_FLASH_RED_W_GREEN:
			if (ISON(LIGHT_RED)) {
				OFF(LIGHT_RED);
			} else {
				ON(LIGHT_RED);
			}
			ON(LIGHT_GREEN);
			break;
		case LIGHT_FLASH_GREEN_W_RED:
			if (ISON(LIGHT_GREEN)) {
				OFF(LIGHT_GREEN);
			} else {
				ON(LIGHT_GREEN);
			}
			ON(LIGHT_RED);
			break;
	}
}

void light_setMode(LIGHT_MODE_e lightMode) {
	if (light_mode != (unsigned char)lightMode) {
		light_mode = (unsigned char)lightMode;
		lightCounter = LIGHT_INTERVAL;
		OFF(LIGHT_GREEN);
		OFF(LIGHT_RED);
	}
}
