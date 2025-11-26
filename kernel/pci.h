#ifndef PCI_H
#define PCI_H

#define PCI_ADDR(bus, dev, func, offset) (\
    0x80000000 | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((offset) & 0xFC))

unsigned int ReadPci32(unsigned char bus, unsigned char dev, unsigned char func, unsigned char offset);

void WritePci32(unsigned char bus, unsigned char dev, unsigned char func, unsigned char offset, unsigned int value);
#endif