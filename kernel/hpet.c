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
#include "smp.h"
#ifdef TALIOS_SCHED_CHECK
#include "keyboard.h"
#endif

#define GCAP_ID 0x0
#define GEN_CONF 0x10
#define GINTR_STA 0x20
#define MAIN_CNT 0xf0

#define TIME0_CONF 0x100
#define TIME0_COMP 0x108

#define TIME1_CONF 0x120
#define TIME1_COMP 0x128

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


HwInterruptT hpet_controller;
extern Time global_time;

void hpet_handler(struct PtRegs * regs, unsigned long nr, unsigned long arg){

    timer_tick();
    scheduler_tick();
    smp_timer_tick();
#ifdef TALIOS_SCHED_CHECK
    keyboard_test_tick(regs->rip);
#endif
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
    entry.DestField.physical.physic_dst = get_lapic_id();

    RegisterIrq(0x22, &entry, &hpet_handler, 0, &hpet_controller, "hpet controller");

    /* Program the 100 Hz period in HPET counter ticks (capability units
     * are femtoseconds per tick). In periodic mode the first comparator
     * write sets the deadline and the second sets its increment. */
    volatile unsigned long *hpet = (volatile unsigned long *)hpet_addr;
    unsigned long period_fs = hpet[GCAP_ID / 8] >> 32;
    unsigned long ticks = 10000000000000UL / period_fs;
    hpet[GEN_CONF / 8] = 0;
    hpet[MAIN_CNT / 8] = 0;
    hpet[TIME0_CONF / 8] = 0x004c;
    hpet[TIME0_COMP / 8] = ticks;
    hpet[TIME0_COMP / 8] = ticks;
    hpet[GEN_CONF / 8] = 3;
    get_cmos_time(&global_time);

    ColorPrintfk(BLUE, BLACK, "hpet drive initialization finish at %d-%d-%d/%d:%d:%d\n", \
                global_time.year, global_time.month, global_time.day, \
                global_time.hour, global_time.minute, global_time.second);
}
void hpet_delay_us(unsigned long us){
    volatile unsigned long *hpet = (volatile unsigned long *)PHY_TO_VIRT(0xfed00000);
    unsigned long ticks = us * 1000000000UL / (hpet[GCAP_ID / 8] >> 32);
    unsigned long start = hpet[MAIN_CNT / 8];
    while(hpet[MAIN_CNT / 8] - start < ticks) __asm__ volatile("pause");
}
