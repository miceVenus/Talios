#include "pci.h"
#include "lib.h"

unsigned int ReadPci32(unsigned char bus, unsigned char dev, unsigned char func, unsigned char offset){

    OUT32b(0xcf8, PCI_ADDR(bus, dev, func, offset));
    mfence();
    return IN32b(0xcfc);
}

void WritePci32(unsigned char bus, unsigned char dev, unsigned char func, unsigned char offset, unsigned int value){
    OUT32b(0xcf8, PCI_ADDR(bus, dev, func, offset));
    mfence();
    OUT32b(0xcfc, value);
    mfence();
}