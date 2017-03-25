/**
 * ringBuffer.h
 *
 *  @author	thagemeier
 */ 

#ifndef _LIB_RING_BUFFER_H_
#define _LIB_RING_BUFFER_H_

/**
*	\brief Internal structure to manage the ring buffer
*/
typedef struct {
	unsigned int readPosition;		/**< Reading position of ring buffer */
	unsigned int writePosition;		/**< Writing position of ring buffer */
	unsigned int size;				/**< Fill level of ring buffer */
	unsigned int maxSize;			/**< Size of buffer */
	unsigned char * buffer;			/**< Pointer to buffer */
} RB_BUFFER_t;

/**
*	\brief Create a new ring buffer
*	@param data Buffer for ring buffer
*	@param buflen Length of buffer
*	@returns Pointer to new ring buffer
*/
RB_BUFFER_t * rb_createBuffer(void * data, unsigned short int buflen);

/**
*	\brief Peek character from ring buffer (do not delete char)
*	@param buf Pointer to ring buffer
*	@param position Position to read (0 based)
*	@returns Character
*/
unsigned char rb_peek(RB_BUFFER_t * buf, unsigned short int position);

/**
*	\brief Delete characters from ring buffer (FIFO)
*	@param buf Pointer to ring buffer
*	@param howMany Number of characters, 0 for all
*/
void rb_delete(RB_BUFFER_t * buf, unsigned short int howMany);

/**
*	\brief Put a char into the ring buffer
*	@param buf Pointer to ring buffer
*	@param whatChar Character
*/
unsigned char rb_put(RB_BUFFER_t * buf, unsigned char whatChar);

/**
*	\brief Get and delete character from ring buffer
*	@param buf Pointer to ring buffer
*	@returns Character read from buffer
*/
unsigned char rb_get(RB_BUFFER_t * buf);

/**
*	\brief Return number of chars in ring buffer
*	@param buf Pointer to ring buffer
*	@returns Number of characters currently in ring buffer
*/
unsigned short int rb_getCount(RB_BUFFER_t * buf);

unsigned char rb_peek_sync(RB_BUFFER_t * buf, unsigned short int position);
void rb_delete_sync(RB_BUFFER_t * buf, unsigned short int howMany);
unsigned char rb_put_sync(RB_BUFFER_t * buf, unsigned char whatChar);
unsigned char rb_get_sync(RB_BUFFER_t * buf);
unsigned short int rb_getCount_sync(RB_BUFFER_t * buf);

/**
*	\brief Calculate how large a buffer for the specified ring buffer capacity must be
*	@param bufferSize The requested buffer capacity in bytes
*/
#define RB_BUFFER_T_BYTE_SIZE(bufferSize)		(sizeof(RB_BUFFER_t) + bufferSize)

#endif
