#include "8259a.h"
#include "printk.h"
#include "lib.h"
#include "interrupt.h"
#include "gate.h"

extern interrupt_t interrupt[24];

void SetMask8259a(unsigned char irq){
    unsigned char mask;

    if(irq < 8){
        mask = IN8b(MASTER_PIC_DATA);
        mask |= (1 << irq);
        OUT8b(MASTER_PIC_DATA, mask);
    }else{
        mask = IN8b(SLAVE_PIC_DATA);
        mask |= (1 << (irq - 8));
        OUT8b(SLAVE_PIC_DATA, mask);
    }
}

void ClearMask8259a(unsigned char irq){
    unsigned char mask;

    if(irq < 8){
        mask = IN8b(MASTER_PIC_DATA);
        mask &= ~(1 << irq);
        OUT8b(MASTER_PIC_DATA, mask);
    }else{
        mask = IN8b(SLAVE_PIC_DATA);
        mask &= ~(1 << (irq - 8));
        OUT8b(SLAVE_PIC_DATA, mask);
    }
}


void Ack8259a(unsigned char irq)
{
	if(irq >= 8) OUT8b(SLAVE_PIC_COMMAND,PIC_EOI);
	OUT8b(MASTER_PIC_COMMAND,PIC_EOI);
}

void Init8259a(){

    for(unsigned int i = 32; i < (32 + 24); i++){
        SetIntrGate(i, 2, interrupt[i-32]);
    }

    ColorPrintfk(BLUE, BLACK, "PIC 8259A Init\n");
    
    // Init Master PIC ICW1 ~ 4 Good Chip Have A "Smart" Protocol To
    // Select Right Register

    OUT8b(MASTER_PIC_COMMAND, 0x11);
    OUT8b(MASTER_PIC_DATA, 0x20);
    OUT8b(MASTER_PIC_DATA, 0x04);
    OUT8b(MASTER_PIC_DATA, 0x01);

    // Init Slave PIC ICW1 ~ 4

    OUT8b(SLAVE_PIC_COMMAND, 0x11);
    OUT8b(SLAVE_PIC_DATA, 0x28);
    OUT8b(SLAVE_PIC_DATA, 0x02);
    OUT8b(SLAVE_PIC_DATA, 0x01);

    // Init Master/Slave PIC OCW1 

    // There Is No Timer Interrupt
    OUT8b(0x21, 0xff);
    OUT8b(0xa1, 0xff);

    sti();
}