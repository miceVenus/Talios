#include "hpet.h"
#include "apic.h"
#include "interrupt.h"
#include "printk.h"
#include "memory.h"
#include "timer.h"
#include "task.h"
#include "lib.h"
#include "softirq.h"
#include "schedule.h"

#define GCAP_ID 0x0
#define GEN_CONF 0x10
#define GINTR_STA 0x20
#define MAIN_CNT 0xf0

#define TIME0_CONF 0x100
#define TIME0_COMP 0x108

#define TIME1_CONF 0x120
#define TIME1_COMP 0x129

#define TIME2_CONF 0x140
#define TIME2_COMP 0x148

#define TIME3_CONF 0x160
#define TIME3_COMP 0x168

#define TIME4_CONF 0x180
#define TIME4_COMP 0x188

#define TIME5_CONF 0x1a0
#define TIME5_COMP 0x1a8

#define TIME6_CONF 0x1c0
#define TIME6_COMP 0x1c8

#define TIME7_CONF 0x1e0
#define TIME7_COMP 0x1e8


unsigned long volatile jiffies = 0;
HwInterruptT hpet_controller;
extern Time global_time;
extern timer_list timer_list_header;
extern scheduler task_scheduler;

struct List *ListNext(struct List* list);

void hpet_handler(struct PtRegs * regs, unsigned long nr, unsigned long arg){
    jiffies++;

    if(ContainerOf(ListNext(&timer_list_header.list), timer_list, list)->expire_jiffies <= jiffies)
    add_softirq_status(TIME_SIRQ);

    struct TaskStruct * current = CURRENT;

    switch (current->priority){
        case 0:
        case 1:
            task_scheduler.CPU_exec_task_jiffies -= 1;
            current->vrun_time += 1;
            break;

        case 2:
            task_scheduler.CPU_exec_task_jiffies -= 2;
            current->vrun_time += 2;
            break;
    }

    if(task_scheduler.CPU_exec_task_jiffies <= 0){
        CURRENT->flags |= NEED_SCHEDULE;
    }

    
}

void hpet_init(){
    unsigned char *hpet_addr = (unsigned char *)PHY_TO_VIRT(0xfed00000);
    IoApicRetEntry  entry;
    BuildController(&hpet_controller);



    entry.vector        = 0x22;
    entry.DelivMode     = DELIV_M_FIXED;
    entry.DestMode      = DEST_M_PHYSICAL;
    entry.IntMask       = IOAPIC_INT_MASKED;
    entry.IntPol        = IOAPIC_INTPOL_H;
    entry.IRR           = IOAPIC_IRR_RESET;
    entry.Trigger       = IOAPIC_TRIGGER_EDGE;
    entry.DelivStatus   = DELIV_S_IDLE;
    entry.reserverd     = 0;
    entry.DestField.physical.reserverd1 = 0;
    entry.DestField.physical.physic_dst = 0;
    entry.DestField.physical.reserverd2 = 0;

    RegisterIrq(0x22, &entry, &hpet_handler, 0, &hpet_controller, "hpet controller");

    *(unsigned long *)(hpet_addr + GEN_CONF) = 3;
    mfence();

    *(unsigned long *)(hpet_addr + TIME0_CONF) = 0x004c;
    mfence();

    *(unsigned long *)(hpet_addr + TIME0_COMP) = 1000000000;
    mfence();

    get_cmos_time(&global_time);
    *(unsigned long *)(hpet_addr + MAIN_CNT) = 0;
    mfence();


    ColorPrintfk(BLUE, BLACK, "hpet drive initialization finish at %d-%d-%d/%d:%d:%d\n", \
                global_time.year, global_time.month, global_time.day, \
                global_time.hour, global_time.minute, global_time.second);
}