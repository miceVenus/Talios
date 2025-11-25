#include "printk.h"
#include "lib.h"
#include "interrupt.h"
#include "gate.h"

extern interrupt_t interrupt[24];

void Init8259a(){

    for(unsigned int i = 32; i < (32 + 24); i++){
        SetIntrGate(i, 2, interrupt[i-32]);
    }

    ColorPrintfk(BLUE, BLACK, "PIC 8259A Init\n");
    
    // Init Master PIC ICW1 ~ 4 Good Chip Have A "Smart" Protocol To
    // Select Right Register

    OUT8b(0x20, 0x11);
    OUT8b(0x21, 0x20);
    OUT8b(0x21, 0x04);
    OUT8b(0x21, 0x01);

    // Init Slave PIC ICW1 ~ 4

    OUT8b(0xa0, 0x11);
    OUT8b(0xa1, 0x28);
    OUT8b(0xa1, 0x02);
    OUT8b(0xa1, 0x01);

    // Init Master/Slave PIC OCW1 

    // There Is No Timer Interrupt
    OUT8b(0x21, 0x01);
    OUT8b(0xa1, 0x00);

    sti();
}