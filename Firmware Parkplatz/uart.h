/*
 * uart.h
 *
 *  Author: thagemeier
 */ 



#ifndef _LIB_UART_H_
#define _LIB_UART_H_

#include "ringBuffer.h"
#include "hardware.h"
#include <avr/pgmspace.h>

/**
*	\brief Structure to keep functions to notify on UART event
*/
typedef struct {
	void * f;					/**< Function pointer to call */
	unsigned int notifyMask;	/**< What events to notify */
	void * infoPtr;				/**< Pointer to pass to function */
} UART_NotifyInfo_t;

typedef unsigned char (*UART_notifyFunction)(void * infoPtr, unsigned char notifyType, unsigned char data);

#define UART_INFO_T_SIZE(nrOfCallbacks)			(sizeof(UART_INFO_t) + (sizeof(UART_NotifyInfo_t) * nrOfCallbacks))

/**
*	\brief Structure to keep UART information
*/
typedef struct {
	USART_t * usart;					/**< Hardware USART */
	RB_BUFFER_t * outputBuffer;			/**< Ring buffer to keep characters to be sent */
	unsigned char flags;				/**< Flags */
	unsigned char stateSend;			/**< Send state of UART */
	unsigned char stateReceive;			/**< Receive state of UART */
	union {								/**< */
		unsigned char readAhead;
		unsigned char doCancelEcho;
	};
	unsigned char exclusive;			/**< Who has exclusive access to the UART */
	unsigned char exclusiveCount;		/**< How many times has somebody got access to the UART (recursive calls) */
	unsigned long int lastEchoByteTime;	/**< Last byte that has been sent */
	unsigned char notifyCount;			/**< Number of functions to notify */
	UART_NotifyInfo_t notifyInfo[];		/**< Notify information */
} UART_INFO_t;

/**
*	\brief UART Events
*/
enum {
	UART_EVENT_RECEIVED = 1,		/**< Character received */
	UART_EVENT_SENT = 2,			/**< Character sent */
	UART_EVENT_BUSY = 3,			/**< UART busy */
	UART_EVENT_TIMEOUT = 4,			/**< UART timeout */
	UART_EVENT_COLLISION = 5,		/**< UART collision */
	UART_EVENT_FRAME_ERROR = 6,		/**< UART framing error */
} UART_EVENT_e;

/**
*	\brief UART notification flags
*/
enum {
	UART_NOTIFY_RECEIVED = 1 << UART_EVENT_RECEIVED,			/**< Character received */
	UART_NOTIFY_SENT = 1 << UART_EVENT_SENT,					/**< Character sent */
	UART_NOTIFY_BUSY = 1 << UART_EVENT_BUSY,					/**< UART busy */
	UART_NOTIFY_TIMEOUT = 1 << UART_EVENT_TIMEOUT,				/**< UART timeout */
	UART_NOTIFY_COLLISION = 1 << UART_EVENT_COLLISION,			/**< UART collision */
	UART_NOTIFY_FRAME_ERROR = 1 << UART_EVENT_FRAME_ERROR,		/**< UART framing error */
} UART_EVENT_NOTIFY_e;

enum {
	UART_FLAG_NO_ECHO_CANCELLATION = 0,
	UART_FLAG_CANCEL_ECHO = 1, 		// In Software um Kollisionsbytes trotzdem rein zu lassen
	UART_FLAG_IGNORE_ECHO = 2, 		// In Hardware um Kollisionsbytes NICHT rein zu lassen
	UART_FLAG_IGNORE_ECHO_EX = 4,	// In Software, Kollisionsbytes ignorierend
	UART_FLAG_PERFORM_ECHO_TEST = 8, // In Software um Echos zu erkennen (z.B. Chiptest des CAN-Transceivers)
	
};

enum {
	UART_STATE_NOINIT = 0,
	UART_STATE_IDLE = 1,
	UART_STATE_SENDING = 2,
	UART_STATE_WAITING = 3,
};

/**
*	\brief Create new UART structure
*	@param usart Hardware USART
*	@param flags Flags to specify UART functionality
*	@param UARTStructBuffer Buffer for UART information structure
*	@param uartStructSize Length of buffer
*	@param Pointer to buffer space
*	@param Length of buffer space
*/
UART_INFO_t * uart_createUART(USART_t * usart, unsigned char flags, void * UARTStructBuffer, unsigned char uartStructSize, void * bufferSpace, unsigned short int bufferSize);

/**
*	\brief Add a function to be notified on UART events
*	@param uart Pointer to UART structure
*	@param infoPtr Pointer to be passed to callback function
*	@param f Callback function
*	@param notifyMask Mask of events to be notified about
*/
unsigned char uart_addModifyNotifyFunction(UART_INFO_t * uart, void * infoPtr, UART_notifyFunction f, unsigned int notifyMask);

/**
*	\brief Signal a "received" interrupt to UART
*	@param uart Pointer to UART structure
*/
void uart_receivedInterrupt(UART_INFO_t * uart);


/**
*	\brief Signal a "data empty" interrupt to UART
*	@param uart Pointer to UART structure
*/
void uart_dataEmptyInterrupt(UART_INFO_t * uart);


/**
*	\brief Signal a "TX complete" interrupt to UART
*	@param uart Pointer to UART structure
*/
void uart_txCompleteInterrupt(UART_INFO_t * uart);


/**
*	\brief Request exclusive access to UART
*	@param uart Pointer to UART structure
*	@param ID Unique ID of caller
*	@returns 1 if successfull, 0 if not
*/
unsigned char uart_getExclusiveAccess(UART_INFO_t * uart, unsigned char ID);

/**
*	\brief Request exclusive access to UART
*	@param uart Pointer to UART structure
*	@param ID Unique ID of caller
*/
void uart_releaseExclusiveAccess(UART_INFO_t * uart, unsigned char ID);

/**
*	\brief Test if an ID has exclusive access to the UART
*	@param uart Pointer to UART structure
*	@param ID ID in question
*	@returns 1 if the ID has exclusive access to the UART, 0 otherwise
*/
unsigned char uart_hasExclusiveAccess(UART_INFO_t * uart, unsigned char ID);

/**
*	\brief Put a character into the UARTs buffer to be sent
*	@param uart Pointer to UART structure
*	@param character Character to be sent
*/
unsigned char uart_putc(UART_INFO_t * uart, unsigned char character);

/**
*	\brief Put a string into the UARTs buffer to be sent
*	@param uart Pointer to UART structure
*	@param buffer Pointer to string buffer
*	@param buflen Length of the string
*/
unsigned char uart_puts(UART_INFO_t * uart, void * buffer, unsigned char buflen);

/**
*	\brief Put a flash string into the UARTs buffer to be sent
*	@param uart Pointer to UART structure
*	@param buffer Pointer to flash string buffer
*	@param buflen Length of the string
*/
unsigned char uart_puts_P(UART_INFO_t * uart, void * buffer, unsigned char buflen);
unsigned char uart_putc_async(UART_INFO_t * uart, unsigned char character);

/**
*	\brief Enable UART hardware
*	@param uart Pointer to UART structure
*	@param speedBaud Speed of the UART in baud
*	@param flags Special flags if required
*/
void uart_enableHardware(UART_INFO_t * uart, unsigned long speedBaud, unsigned char flags);

/**
*	\brief Disable UART hardware
*	@param uart Pointer to UART structure
*/
void uart_disableHardware(UART_INFO_t * uart);

/**
*	\brief Test if the UART is sending
*	@param uart Pointer to UART structure
*	@returns 1 if the UART is currently sending, 0 if not
*/
unsigned char uart_isBusy(UART_INFO_t * uart);
unsigned int uart_getTransmissionDelayMs(unsigned long speedBaud, unsigned int rxlen);
void uart_forwardNotify(UART_INFO_t * uart, unsigned char event, unsigned char data, void * afterPtr);
unsigned char uart_outputValue(UART_INFO_t * uart, unsigned int value);
void uart_changeSpeed(UART_INFO_t * uart, unsigned long speedBaud);
unsigned char uart_performEchoTest(UART_INFO_t * uart, unsigned char character);

#define UART_INFO_T_BYTE_SIZE(notifyItemCount)		(sizeof(UART_INFO_t) + (sizeof(UART_NotifyInfo_t) * notifyItemCount))

#endif /* UART_H_ */
