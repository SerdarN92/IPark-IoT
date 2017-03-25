#ifndef _AES_H_
#define _AES_H_

#define AES_BUFFER_SIZE		16		/** Size of the AES-128 state buffer in bytes */
#define AES_KEY_SIZE		16		/** Size of the AES-128 key in bytes */

/**
* \brief Encrypts or decrypts data in-place by the given sequence using AES CTR as stream cipher
* @param[in,out] data Pointer to data buffer
* @param[in] len Length of data in bytes
* @param[in] msgSrc Pointer to message source
* @param[in] msgDest Pointer to message destination
* @param[in] sequence Pointer to sequence
*/
void aes_encryptDecryptData(unsigned char * data, unsigned int len, unsigned char * msgSrc, unsigned char * msgDest, unsigned char * sequence);


#endif
