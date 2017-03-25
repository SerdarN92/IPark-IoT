
#ifndef _LIB_HARDWARE_H_
#define _LIB_HARDWARE_H_

#include <avr/io.h>

#define F_TIMER		F_CPU

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE		512
#endif

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

/**
*	\brief Information about current hardware
*/
typedef struct {
	unsigned char * usedHwDef;	/**< Pointer to pin definition */
	unsigned char memType;		/**< What memory does the pointer refer to? */
	unsigned char hwDefLen;		/**< Length of pin definition */
	USART_t * usartPC;			/**< UART to use for communication */
} HW_DEF_t;

extern HW_DEF_t hwDef;

void hw_init(); /**< \brief Initialize hardware */

/**
*	\brief Return whether a named pin is available
*	@param pinID ID of Pin
*/
unsigned char hw_isPinAvailable(unsigned char pinID);

/**
*	\brief Add a pin definition to a char array (encode specified pin to 8 bit)
*	@param x (Not used, text description)
*	@param y Reference to port (PORTA, PORTB, ..)
*	@param z Pin number (0-7) on pin
*/
#define HW_ASSIGN_PORT_PIN(x, y, z)	( \
										(((unsigned int)&(y)) >= 0x600) \
										? ((HW_GET_PORTID(y) | (z & 0xf)) + 1) \
										: 0 \
									)			/* x wird ignoriert, hier können wir was beliebiges angeben..? */

#define CONCAT_X(a, b)				a ## b
#define CONCAT(a, b)				CONCAT_X(a, b)
#define CONCAT_X3(a, b, c)			a ## b ## c
#define CONCAT3(a, b, c)			CONCAT_X3(a, b, c)
#define CONCAT_X4(a, b, c, d)		a ## b ## c ## d
#define CONCAT4(a, b, c, d)			CONCAT_X4(a, b, c, d)


// Generelle Macros die benutzt werden um die Pins besser ansprechen zu können..

#define CONCAT_PORT_OUTCLR(x)		CONCAT(x, _PORT.OUTCLR)
#define CONCAT_PORT_OUTSET(x)		CONCAT(x, _PORT.OUTSET)
#define CONCAT_PORT_OUT(x)			CONCAT(x, _PORT.OUT)
#define CONCAT_PORT_IN(x)			CONCAT(x, _PORT.IN)
#define CONCAT_PORT_DIR(x)			CONCAT(x, _PORT.DIR)
#define CONCAT_PORT_DIRSET(x)		CONCAT(x, _PORT.DIRSET)
#define CONCAT_PORT_DIRCLR(x)		CONCAT(x, _PORT.DIRCLR)
#define CONCAT_PORT(x)				CONCAT(x, _PORT)
#define CONCAT_PIN(x)				CONCAT(x, _PIN)
#define CONCAT_DRIVEMODE(x)			CONCAT(x, _DRIVEMODE)
#define CONCAT_TESTMODE(x)			CONCAT(x, _TESTMODE)
#define CONCAT_PORT_PINCTRL(x, y)	CONCAT4(x, _PORT.PIN, y, CTRL)

#define SET_OPC(func, mode)			CONCAT_PORT_PINCTRL(func, CONCAT_PIN(func)) = ((CONCAT_PORT_PINCTRL(func, CONCAT_PIN(func)) & ~PORT_OPC_gm) | mode)

void hw_setOn(unsigned char pinID);
void hw_setOff(unsigned char pinID);
unsigned char hw_isOn(unsigned char pinID);
void hw_setOutput(unsigned char pinID);
void hw_setInput(unsigned char pinID);
void hw_setOPC(unsigned char pinID, unsigned char opc);
void hw_setInvert(unsigned char pinID, unsigned char do_invert);
void hw_setLimitSlewRate(unsigned char pinID, unsigned char do_limit);
unsigned char hw_getPinData(unsigned char pinID, PORT_t ** port, unsigned char ** pinctrl, unsigned char * pin);
unsigned char hw_resolvePinData(unsigned char hwPin, PORT_t ** port, unsigned char ** pinctrl, unsigned char * pin);

// Funktion, um die Funktion der FUNCID zuzuweisen:
#define CONCAT_FUNCID(x)			CONCAT_FUNCID_X(x)
#define CONCAT_FUNCID_X(x)			CONCAT(x, _FUNCID)

// Funktionen für die Hardware wie sie auch jetzt schon überall verwendet werden:
#define SET_PULLUP(func)			hw_setOPC(CONCAT_FUNCID(func), PORT_OPC_PULLUP_gc)
#define SET_PULLDOWN(func)			hw_setOPC(CONCAT_FUNCID(func), PORT_OPC_PULLDOWN_gc)
#define SET_TOTEM(func)				hw_setOPC(CONCAT_FUNCID(func), PORT_OPC_TOTEM_gc)
#define SET_BUSKEEPER(func)			hw_setOPC(CONCAT_FUNCID(func), PORT_OPC_BUSKEEPER_gc)
#define SET_INPUT(func)				hw_setInput(CONCAT_FUNCID(func))
#define SET_OUTPUT(func)			hw_setOutput(CONCAT_FUNCID(func))
#define SET_INVERT(func, yn)		hw_setInvert(CONCAT_FUNCID(func), yn)
#define SET_SLIMIT(func, yn)		hw_setInvert(CONCAT_FUNCID(func), yn)
#define ON(func)					hw_setOn(CONCAT_FUNCID(func))
#define OFF(func)					hw_setOff(CONCAT_FUNCID(func))
#define ISON(func)					hw_isOn(CONCAT_FUNCID(func))

// Tabelle für die Zuordnung der Ports/Pins:

#define SET_OPC(func, mode)			CONCAT_PORT_PINCTRL(func, CONCAT_PIN(func)) = ((CONCAT_PORT_PINCTRL(func, CONCAT_PIN(func)) & ~PORT_OPC_gm) | mode)

#define HW_GET_PORTID(x)			(((((unsigned int)&(x)) - 0x600) / 0x20) << 4)

#define HW_ASSIGN(x)				HW_ASSIGN_X(CONCAT_PORT(x), CONCAT_PIN(x))
#define HW_ASSIGN_X(x, y)			(HW_GET_PORTID(x) | y) + 1,
#define HW_ASSIGN_EMPTY(x)			0xff,


#define MEM_TYPE_NOTFOUND	0
#define MEM_TYPE_PROGMEM	1
#define MEM_TYPE_RAM		2
#define MEM_TYPE_DEFAULT	3


enum {
	NO_FUNCTION_FUNCID,
	LED0_FUNCID,
	LED1_FUNCID,
	LED2_FUNCID,
	LED3_FUNCID,
	LED4_FUNCID,
	LED5_FUNCID,
	LED6_FUNCID,
	LED7_FUNCID,
	KEY0_FUNCID,
	KEY1_FUNCID,
	KEY2_FUNCID,
	KEY3_FUNCID,
	KEY4_FUNCID,
	KEY5_FUNCID,
	KEY6_FUNCID,
	KEY7_FUNCID,
	PC_UART_TX_FUNCID,
	PC_UART_RX_FUNCID,
	EBI_WE_FUNCID,
	EBI_CAS_FUNCID,
	EBI_RAS_FUNCID,
	EBI_DQM_FUNCID,
	EBI_BA0_FUNCID,
	EBI_BA1_FUNCID,
	EBI_CKE_FUNCID,
	EBI_CLK_FUNCID,
	EBI_A0_FUNCID,
	EBI_A1_FUNCID,
	EBI_A2_FUNCID,
	EBI_A3_FUNCID,
	EBI_A4_FUNCID,
	EBI_A5_FUNCID,
	EBI_A6_FUNCID,
	EBI_A7_FUNCID,
	EBI_A8_FUNCID,
	EBI_A9_FUNCID,
	EBI_A10_FUNCID,
	EBI_A11_FUNCID,
	SERVO_A_FUNCID,
	DISTANCE_PWM_FUNCID,
	DISTANCE_START_MEASURING_FUNCID,
	LIGHT_RED_FUNCID,
	LIGHT_GREEN_FUNCID,
} PIN_IDs;






/*
#include <stdarg.h>
#include <avr/pgmspace.h>
void console_printf_P(const prog_char * format, ...);
*/

#endif // #ifdef _HARDWARE_H_

