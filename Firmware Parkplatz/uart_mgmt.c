#include "hardware.h"
#include "uart_mgmt.h"
//#include "mod_communication.h"
#include <avr/interrupt.h>
#include "uart.h"
#include "systimer.h"
#include "hardware.h"
#include "common.h"
#include "extram.h"

UART_INFO_t * uartPC;

static unsigned char uart1_mgmt[UART_INFO_T_SIZE(5) + 1] EXTRAM; // 4 Callbacks max
static unsigned char uart1_buffer[2000] EXTRAM;

void uartmgmt_setupUARTs(USART_t * pcUSART) {
	// Make connections of UARTs:
	uartPC = uart_createUART(pcUSART, 0, &uart1_mgmt[0], sizeof(uart1_mgmt), &uart1_buffer[0], sizeof(uart1_buffer));
	uart_enableHardware(uartPC, 57600, 0);
}

ISR (USARTC0_TXC_vect) {
	USART_t * usart = &USARTC0;
	if (hwDef.usartPC == usart)
		uart_txCompleteInterrupt(uartPC);
}

ISR (USARTC0_DRE_vect) {
	USART_t * usart = &USARTC0;
	if (hwDef.usartPC == usart)
		uart_dataEmptyInterrupt(uartPC);
}

ISR (USARTC0_RXC_vect) {
	USART_t * usart = &USARTC0;
	if (hwDef.usartPC == usart)
		uart_receivedInterrupt(uartPC);
}


