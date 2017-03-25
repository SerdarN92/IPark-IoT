#include <avr/io.h>
#include "hardware.h"
#include <avr/pgmspace.h>

HW_DEF_t hwDef;

// Aus der UserSignature den richtigen Pin auslesen:
unsigned char hw_getPinData(unsigned char pinID, PORT_t ** port, unsigned char ** pinctrl, unsigned char * pin) {
	unsigned char hwPin;
	// Heraussuchen, welchen Pin wir meinen:
	if (pinID < hwDef.hwDefLen) {
		switch (hwDef.memType) {
			case MEM_TYPE_NOTFOUND:
				return 0;
			case MEM_TYPE_PROGMEM:
				hwPin = pgm_read_byte(&hwDef.usedHwDef[pinID]);
				break;
			case MEM_TYPE_RAM:
				hwPin = hwDef.usedHwDef[pinID];
				break;
			case MEM_TYPE_DEFAULT:
				return 0;
			default:
				return 0;
		}
		return hw_resolvePinData(hwPin, port, pinctrl, pin);
	}
	return 0;
}

unsigned char hw_resolvePinData(unsigned char hwPin, PORT_t ** port, unsigned char ** pinctrl, unsigned char * pin) {
	PORT_t * myPort;
	if (hwPin && (hwPin != 0xff)) {
		hwPin--;
		// Richtigen Port finden:
		myPort = (PORT_t*)(((unsigned int)&PORTA) + (unsigned int)((hwPin >> 4) * 0x20));
		// Wenn gewünscht, den Port nach draußen geben:
		if (port)
			*port = myPort;
		// Wenn gewünscht, den PINCTRL-Block nach draußen geben:
		if (pinctrl)
			*pinctrl = ((unsigned char *)&(myPort->PIN0CTRL)) + (hwPin & 0xf);
		// Wenn gewünscht auch noch die Pin-Nummer:
		if (pin)
			*pin = hwPin & 0xf;
		return hwPin + 1; // Add one to not give back "0" (false "pin not available" indication)
	}
	return 0;
}

void hw_setOn(unsigned char pinID) {
	PORT_t * port;
	unsigned char pin;
	if (hw_getPinData(pinID, &port, 0, &pin))
		port->OUTSET = (1 << pin);
}

void hw_setOff(unsigned char pinID) {
	PORT_t * port;
	unsigned char pin;
	if (hw_getPinData(pinID, &port, 0, &pin))
		port->OUTCLR = (1 << pin);
}

unsigned char hw_isOn(unsigned char pinID) {
	PORT_t * port;
	unsigned char pin;
	if (hw_getPinData(pinID, &port, 0, &pin))
		return (port->IN >> pin) & 1;
	return 0;
}

void hw_setOutput(unsigned char pinID) {
	PORT_t * port;
	unsigned char pin;
	if (hw_getPinData(pinID, &port, 0, &pin))
		port->DIRSET = (1 << pin);
}

void hw_setInput(unsigned char pinID) {
	PORT_t * port;
	unsigned char pin;
	if (hw_getPinData(pinID, &port, 0, &pin))
		port->DIRCLR = (1 << pin);
}

void hw_setOPC(unsigned char pinID, unsigned char opc) {
	unsigned char * pinctrl;
	if (hw_getPinData(pinID, 0, &pinctrl, 0))
		*pinctrl = (*pinctrl & ~(PORT_OPC_gm)) | opc;
}

void hw_setInvert(unsigned char pinID, unsigned char do_invert) {
	unsigned char * pinctrl;
	if (hw_getPinData(pinID, 0, &pinctrl, 0)) {
		if (do_invert) {
			*pinctrl |= PORT_INVEN_bm;
		} else {
			*pinctrl &= ~(PORT_INVEN_bm);
		}
	}
}

void hw_setLimitSlewRate(unsigned char pinID, unsigned char do_limit) {
	unsigned char * pinctrl;
	if (hw_getPinData(pinID, 0, &pinctrl, 0)) {
		if (do_limit) {
			*pinctrl |= PORT_SRLEN_bm;
		} else {
			*pinctrl &= ~(PORT_SRLEN_bm);
		}
	}
}

unsigned char hw_isPinAvailable(unsigned char pinID) {
	return hw_getPinData(pinID, 0, 0, 0);
}

#define NONE						(*(PORT_t *)0x500)

#ifndef HW_GET_PORTID
#define HW_GET_PORTID(x)			(((((unsigned int)&(x)) - 0x600) / 0x20) << 4)
#endif
#ifndef HW_ASSIGN
#define HW_ASSIGN(x)				HW_ASSIGN_X(CONCAT_PORT(x), CONCAT_PIN(x))
#endif
#ifndef HW_ASSIGN_X
#define HW_ASSIGN_X(x, y)			(HW_GET_PORTID(x) | y) + 1,
#endif
#ifndef HW_ASSIGN_EMPTY
#define HW_ASSIGN_EMPTY(x)			0xff,
#endif

/*
** ===============================================================================================
** Achtung: Das erste Argument dient ausschließlich der Referenz und wird vom Makro raus geworfen!
** ===============================================================================================
*/

/**
*	\brief Pin definition for hardware platform
*/
static unsigned char hwDefinition_iPark[] PROGMEM = {
	HW_ASSIGN_PORT_PIN(NO_FUNCTION_FUNCID,		NONE,  0),
	HW_ASSIGN_PORT_PIN(LED0_FUNCID,				PORTE, 0),
	HW_ASSIGN_PORT_PIN(LED1_FUNCID,				PORTE, 1),
	HW_ASSIGN_PORT_PIN(LED2_FUNCID,				PORTE, 2),
	HW_ASSIGN_PORT_PIN(LED3_FUNCID,				PORTE, 3),
	HW_ASSIGN_PORT_PIN(LED4_FUNCID,				PORTE, 4),
	HW_ASSIGN_PORT_PIN(LED5_FUNCID,				PORTE, 5),
	HW_ASSIGN_PORT_PIN(LED6_FUNCID,				PORTE, 6),
	HW_ASSIGN_PORT_PIN(LED7_FUNCID,				PORTE, 7),
	HW_ASSIGN_PORT_PIN(KEY0_FUNCID,				PORTD, 0),
	HW_ASSIGN_PORT_PIN(KEY1_FUNCID,				PORTD, 1),
	HW_ASSIGN_PORT_PIN(KEY2_FUNCID,				PORTD, 2),
	HW_ASSIGN_PORT_PIN(KEY3_FUNCID,				PORTD, 3),
	HW_ASSIGN_PORT_PIN(KEY4_FUNCID,				PORTD, 4),
	HW_ASSIGN_PORT_PIN(KEY5_FUNCID,				PORTD, 5),
	HW_ASSIGN_PORT_PIN(KEY6_FUNCID,				PORTR, 0), // Gegengewicht für SMPTE-Output beim AM-Prototyp
	HW_ASSIGN_PORT_PIN(KEY7_FUNCID,				PORTR, 1),
	
	HW_ASSIGN_PORT_PIN(PC_UART_TX_FUNCID,		PORTC, 3),
	HW_ASSIGN_PORT_PIN(PC_UART_RX_FUNCID,		PORTC, 2),
	
	HW_ASSIGN_PORT_PIN(EBI_WE_FUNCID,			PORTH, 0),
	HW_ASSIGN_PORT_PIN(EBI_CAS_FUNCID,			PORTH, 1),
	HW_ASSIGN_PORT_PIN(EBI_RAS_FUNCID,			PORTH, 2),
	HW_ASSIGN_PORT_PIN(EBI_DQM_FUNCID,			PORTH, 3),
	HW_ASSIGN_PORT_PIN(EBI_BA0_FUNCID,			PORTH, 4),
	HW_ASSIGN_PORT_PIN(EBI_BA1_FUNCID,			PORTH, 5),
	HW_ASSIGN_PORT_PIN(EBI_CKE_FUNCID,			PORTH, 6),
	HW_ASSIGN_PORT_PIN(EBI_CLK_FUNCID,			PORTH, 7),
	HW_ASSIGN_PORT_PIN(EBI_A0_FUNCID,			PORTK, 0),
	HW_ASSIGN_PORT_PIN(EBI_A1_FUNCID,			PORTK, 1),
	HW_ASSIGN_PORT_PIN(EBI_A2_FUNCID,			PORTK, 2),
	HW_ASSIGN_PORT_PIN(EBI_A3_FUNCID,			PORTK, 3),
	HW_ASSIGN_PORT_PIN(EBI_A4_FUNCID,			PORTK, 4),
	HW_ASSIGN_PORT_PIN(EBI_A5_FUNCID,			PORTK, 5),
	HW_ASSIGN_PORT_PIN(EBI_A6_FUNCID,			PORTK, 6),
	HW_ASSIGN_PORT_PIN(EBI_A7_FUNCID,			PORTK, 7),
	HW_ASSIGN_PORT_PIN(EBI_A8_FUNCID,			PORTJ, 4),
	HW_ASSIGN_PORT_PIN(EBI_A9_FUNCID,			PORTJ, 5),
	HW_ASSIGN_PORT_PIN(EBI_A10_FUNCID,			PORTJ, 6),
	HW_ASSIGN_PORT_PIN(EBI_A11_FUNCID,			PORTJ, 7),

	HW_ASSIGN_PORT_PIN(SERVO_A_FUNCID,			PORTC, 0),
	
	HW_ASSIGN_PORT_PIN(DISTANCE_PWM_FUNCID,					PORTC, 7),
	HW_ASSIGN_PORT_PIN(DISTANCE_START_MEASURING_FUNCID,		PORTC, 6),
	
	HW_ASSIGN_PORT_PIN(LIGHT_RED_FUNCID,		PORTC, 4),
	HW_ASSIGN_PORT_PIN(LIGHT_GREEN_FUNCID,		PORTC, 5),

};

/**
*	\brief Definition of pins having to be set to PULLDOWN
*/
static unsigned char hwDefinition_iPark_WeakPullDown[] PROGMEM = {
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTB, 6),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTC, 0),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTC, 5),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTE, 0),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTE, 2),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTF, 0),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTF, 1),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTQ, 2),
	HW_ASSIGN_PORT_PIN(NOT_USED,			PORTQ, 3),
};


































unsigned char hw_getHardwareVersion(unsigned char defaultVersion) {
	unsigned char c1;
	unsigned char c2;
	unsigned int hwVersion;
	
	c1 = pgm_read_byte_far(BOOT_SECTION_START - 3);	// Byte-3 vor dem Bootloader
	c2 = pgm_read_byte_far(BOOT_SECTION_START - 2);	// Byte-2 vor dem Bootloader
	
	hwVersion = ((unsigned int)c2 << 8) | c1;
	if (c1 == (c2 ^ 0xff))
		return c1;
	
	// Alternativ müssen wir im Bootloader-Bereich nachgucken. Das KANN allerdings ein Problem werden, wenn die Fuses gesperrt sind..
	c1 = pgm_read_byte_far(0x201f4UL);	// Byte direkt nach dem Vektor-Table des Bootloaders
	c2 = pgm_read_byte_far(0x201f5UL);	// +1 Byte (wegen Alignment frei gehalten)
	
	hwVersion = ((unsigned int)c2 << 8) | c1;
	// hwVersion 0x2411 ist der normale Boot-Code bevor wir die HW-Version in den Bootloader integriert hatten! Der Wert muss also auf Version "0" führen:
	if (hwVersion == 0x2411)
		return defaultVersion;
	if (hwVersion == 0xffff)	// Kein Bootloader da
		return defaultVersion;
	// c1 exor c2 muss genau 0xff sein (c1 genau komplement von c2)
	if (c1 != (c2 ^ 0xff))
		return 0xff;
	return c1;
}

void hw_switchHardwareDefinitions(void) {
	unsigned char * pinctrl;
	unsigned char * doInitPulldown = 0;
	unsigned char doInitPulldownLen = 0;
	unsigned char i;
	
	hwDef.memType = MEM_TYPE_PROGMEM;
	hwDef.usedHwDef = &hwDefinition_iPark[0];
	hwDef.hwDefLen = sizeof(hwDefinition_iPark) / sizeof(hwDefinition_iPark[0]);
	doInitPulldown = &hwDefinition_iPark_WeakPullDown[0];
	doInitPulldownLen = sizeof(hwDefinition_iPark_WeakPullDown) / sizeof(hwDefinition_iPark_WeakPullDown[0]);
	hwDef.usartPC = &USARTC0;
	//hwDef.adcInfo = (ADC_CTRL_t*)&hwDefinitions_AM1_ADCInfo[0];
	
	// Nicht benutzte Pins hier schon auf Pulldown schalten:
	for (i=0;i<doInitPulldownLen;i++) {
		// Raussuchen wo der Pin-Control-Block liegt:
		if (hw_getPinData(doInitPulldown[i], 0, &pinctrl, 0)) {
			// Und dann den Pin-Control-Block entsprechend bearbeiten um den Pulldown zu erreichen:
			*pinctrl = (*pinctrl & ~(PORT_OPC_gm)) | PORT_OPC_PULLDOWN_gc;
		}
	}
}

void hw_init() {
	hw_switchHardwareDefinitions();
	
	// Pins richtig setzen

	// LEDs:
	SET_INVERT(LED0, 1); SET_OUTPUT(LED0);
	SET_INVERT(LED1, 1); SET_OUTPUT(LED1);
	SET_INVERT(LED2, 1); SET_OUTPUT(LED2);
	SET_INVERT(LED3, 1); SET_OUTPUT(LED3);
	SET_INVERT(LED4, 1); SET_OUTPUT(LED4);
	SET_INVERT(LED5, 1); SET_OUTPUT(LED5);
	SET_INVERT(LED6, 1); SET_OUTPUT(LED6);
	SET_INVERT(LED7, 1); SET_OUTPUT(LED7);
	
	// Buttons:
	SET_PULLUP(KEY0); SET_INVERT(KEY0, 1);
	SET_PULLUP(KEY1); SET_INVERT(KEY1, 1);
	SET_PULLUP(KEY2); SET_INVERT(KEY2, 1);
	SET_PULLUP(KEY3); SET_INVERT(KEY3, 1);
	SET_PULLUP(KEY4); SET_INVERT(KEY4, 1);
	SET_PULLUP(KEY5); SET_INVERT(KEY5, 1);
	SET_PULLUP(KEY6); SET_INVERT(KEY6, 1);
	SET_PULLUP(KEY7); SET_INVERT(KEY7, 1);
	
	// UART zum PC:
	SET_INPUT(PC_UART_RX);
	SET_PULLUP(PC_UART_RX);
	SET_OUTPUT(PC_UART_TX);
	ON(PC_UART_TX);
	
	// EBI:
	ON(EBI_WE);
	SET_OUTPUT(EBI_WE);
	ON(EBI_CAS);
	SET_OUTPUT(EBI_CAS);
	ON(EBI_RAS);
	SET_OUTPUT(EBI_RAS);
	ON(EBI_DQM);
	SET_OUTPUT(EBI_DQM);
	OFF(EBI_BA0);
	SET_OUTPUT(EBI_BA0);
	OFF(EBI_BA1);
	SET_OUTPUT(EBI_BA1);
	OFF(EBI_CKE);
	SET_OUTPUT(EBI_CKE);
	OFF(EBI_CLK);
	SET_OUTPUT(EBI_CLK);
	
	SET_OUTPUT(EBI_A0);
	SET_OUTPUT(EBI_A1);
	SET_OUTPUT(EBI_A2);
	SET_OUTPUT(EBI_A3);
	SET_OUTPUT(EBI_A4);
	SET_OUTPUT(EBI_A5);
	SET_OUTPUT(EBI_A6);
	SET_OUTPUT(EBI_A7);
	SET_OUTPUT(EBI_A8);
	SET_OUTPUT(EBI_A9);
	SET_OUTPUT(EBI_A10);
	SET_OUTPUT(EBI_A11);

	SET_OUTPUT(SERVO_A);
	SET_OUTPUT(DISTANCE_START_MEASURING);

	SET_OUTPUT(LIGHT_RED);
	SET_OUTPUT(LIGHT_GREEN);
}


