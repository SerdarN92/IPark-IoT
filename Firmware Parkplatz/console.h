#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <avr/pgmspace.h>

/**
* \brief Debug output, same as printf
*/
void uart_printf(const char* format, ...);

/**
* \brief Debug output, same as printf from flash memory
*/
void uart_printf_P(const prog_char *format, ...);


#endif
