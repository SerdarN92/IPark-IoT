#ifndef _UART_MGMT_H_
#define _UART_MGMT_H_

#include "hardware.h"
#include "uart.h"


extern UART_INFO_t * uartPC;

void uartmgmt_setupUARTs(USART_t * pcUSART);


#endif
