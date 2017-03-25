#ifndef _COAP_ENDPOINT_H_
#define _COAP_ENDPOINT_H_

#include "communication.h"

/**
* \brief Process incoming CoAP message
* @param msg Pointer to CoAP message
*/
void ce_processIncomingMessage(MESSAGE_t * msg);
/**
* \brief Setup endpoints for CoAP library
*/
void ep_setup();


#endif
