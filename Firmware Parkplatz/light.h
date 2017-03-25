#ifndef _LIGHT_H_
#define _LIGHT_H_

/**
*	\brief Signal colors/blinking types
*/
typedef enum {
	LIGHT_OFF,					/**< Light off */
	LIGHT_RED,					/**< Light red */
	LIGHT_GREEN,				/**< Light green */
	LIGHT_FLASH_RED,			/**< Light flashing red */
	LIGHT_FLASH_GREEN,			/**< Light flashing green */
	LIGHT_FLASH_RED_W_GREEN,	/**< Light red steady with green flashing */
	LIGHT_FLASH_GREEN_W_RED,	/**< Light green steady with red flashing */
} LIGHT_MODE_e;

/**
*	\brief Initialize signal controller
*/
void light_init();

/**
*	\brief Set signal mode
*	@param lightMode Light blinking mode
*/
void light_setMode(LIGHT_MODE_e lightMode);

#endif
