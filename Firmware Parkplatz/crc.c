#include "crc.h"

#define CRC16MASK	0x8811	// CRC-CCITT-16

unsigned short crc_calcCRC16r(unsigned short crc, unsigned short c) {
  unsigned char i;
  for(i=0;i<8;i++) {
    if ((crc ^ c) & 1) {
      crc = (crc >> 1) ^ CRC16MASK;
    } else {
      crc >>= 1;
    }
    c >>= 1;
  }
  return crc;
}
