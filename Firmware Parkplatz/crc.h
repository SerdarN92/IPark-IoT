#ifndef _CRC_H_
#define _CRC_H_

/**
*	\brief Iteratively calculate the CRC checksum for a message (CRC-CCIT)
*	@param[in] crc CRC calculated so far
*	@param[in] c New character to be taken into account
*	@returns New CRC value (final CRC or used as crc input value for next byte)
*/
unsigned short crc_calcCRC16r(unsigned short crc, unsigned short c);

#endif
