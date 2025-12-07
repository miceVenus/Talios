#ifndef _8259A_H
#define _8259A_H

#define MASTER_PIC		    0x20		/* IO base address for master PIC */
#define SLAVE_PIC		    0xa0		/* IO base address for slave PIC */
#define MASTER_PIC_COMMAND	MASTER_PIC
#define MASTER_PIC_DATA	    (MASTER_PIC+1)
#define SLAVE_PIC_COMMAND	SLAVE_PIC
#define SLAVE_PIC_DATA	    (SLAVE_PIC+1)

#define PIC_EOI             0x20

void Init8259a();
void SetMask8259a(unsigned char irq);
void ClearMask8259a(unsigned char irq);
void Ack8259a(unsigned char irq);
#endif