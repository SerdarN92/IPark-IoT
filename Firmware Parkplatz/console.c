#include "console.h"
#include <stdio.h>
#include <stdarg.h>
#include <avr/pgmspace.h>
#include "common.h"
#include "uart_mgmt.h"
#include "uart.h"


#define debugUART uartPC

void uart_printf(const char* format, ...) {
	char debugBuffer[128];
	unsigned char count;
	if (!debugUART)
		return;
	va_list argptr;
	va_start(argptr, format);
	count = vsnprintf(&debugBuffer[0], sizeof(debugBuffer), format, argptr);
	va_end(argptr);
	uart_puts(debugUART, &debugBuffer[0], count);
	wait(5);
}

void uart_printf_P(const prog_char *format, ...) {
	char debugBuffer[128];
	unsigned char count;
	if (!debugUART)
		return;
	va_list argptr;
	va_start(argptr, format);
	count = vsnprintf_P(&debugBuffer[0], sizeof(debugBuffer), format, argptr);
	va_end(argptr);
	uart_puts(debugUART, &debugBuffer[0], count);
	wait(5); // Platz für 28 Zeichen.
}
