#ifndef APIC_H
#define APIC_H

struct IoApicMap{
    unsigned int    PhysicalAddr;
    unsigned char   * VirtualIndexAddr;
    unsigned int    * VirtualDataAddr;
    unsigned int    * VirtualEoiAddr;
};

void InitLocalApic();
void InitIoApic();
void IoApicPageTableRemap();
#endif