#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "uart.h"

// We do not use port numbers in the protocol right now:
#define USE_PORT_NUMBERS
#undef  USE_PORT_NUMBERS


#define SYNCBYTE				0xf3	/**< Start byte of each message */
#define MAX_PAYLOAD_LENGTH		(1500)	/**< Maximum payload of each message */
#define HEADER_LENGTH			(4+4+1+1+6+2)
#define MAXMESSAGELENGTH		(MAX_PAYLOAD_LENGTH + HEADER_LENGTH)

#define MSG_FLAG_SUBPROTOCOL		1	/**< Flag: Sub protocol specified in message */
#define MSG_FLAG_ENCRYPTED			2	/**< Flag: Message payload is encrypted */

// Internal flag: Sequence number is valid
#define MSG_IFLAG_VALIDSEQUENCE		1

/** Layer 3-4 Internal message structure */
typedef struct {
	unsigned long int msgDest;	/**< Message destination address */
	unsigned long int msgSrc;	/**< Message source address */
	unsigned short int srcPort;	/**< Message destination port */
	unsigned short int dstPort;	/**< Message source port */
	unsigned char flags;		/**< Message flags */
	unsigned char * sequence;	/**< Message sequence for encryption */
	unsigned char subProtocol;	/**< Subprotocol of layer 5 */
	unsigned char * content;	/**< Pointer to message content */
	unsigned short int len;		/**< Message payload/content */
} MESSAGE_t;

/**
*	\brief COM Event of message, called by uart.c. Adds byte to uncoming queue.
*/
unsigned char com_uartEvent(void * infoPtr, unsigned char event, unsigned char data);

/**
*	\brief Send out layer 3-4 message
* @param uart UART to send the message with
* @param msg Message to send out
*/
void com_sendProtocolMessage(UART_INFO_t * uart, MESSAGE_t * msg);

/**
*	\brief Hook communication to given UART
*	@param[in] uart UART to connect to
*/
void com_init(UART_INFO_t * uart);

#endif
