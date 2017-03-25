
// __attribute__ ((naked)) 
void memoryTransferDMA(unsigned long from, unsigned long to, unsigned char howMany);
void __attribute__ ((naked)) memoryTransfer(unsigned long from, unsigned long to, unsigned char howMany);
void __attribute__ ((naked)) memoryTransferNoDMA(unsigned long from, unsigned long to, unsigned char howMany);
void __attribute__ ((naked)) memoryZero(unsigned long to, unsigned char howMany);

void memoryTransferLong(unsigned long from, unsigned long to, unsigned int howMany);
void memoryZeroLong(unsigned long to, unsigned int howMany);

