#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "lib.h"
#include "interrupt.h"
#include "task.h"
#include "cpu.h"
#include "8259a.h"
#include "apic.h"
#include "keyboard.h"
#include "floppy.h"
#include "smp.h"
#include "timer.h"
#include "hpet.h"
#include "softirq.h"
#include "schedule.h"
#include "test/memory_test.h"


void BRK(){

}

extern unsigned long _stack_start;
// extern unsigned int TssTable[];
extern struct GlobalMemManager MMS;
extern Time global_time;
extern struct TssStruct InitTss[NR_CPUS];

void main(){

    IcrEntry icr_entry = {0};
    unsigned long ist_ptr = 0;

    PrintkInit();
    
    LTR(10);  // Check And Reloade TR

    SetTss( (unsigned int *)&InitTss[0], _stack_start, _stack_start, _stack_start, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00);

    SysVectorInit();
        
    CpuInit();

    // memory init start

    InitMemory();

    SlabCacheInit();

    InitPageTable();

    // memory init end

    ist_ptr = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
    ((struct TaskStruct*)(ist_ptr - STACK_SIZE))->cpu_id = 0;
    InitTss[0].ist1 = ist_ptr;
    InitTss[0].ist2 = ist_ptr;
    InitTss[0].ist3 = ist_ptr;
    InitTss[0].ist4 = ist_ptr;
    InitTss[0].ist5 = ist_ptr;
    InitTss[0].ist6 = ist_ptr;
    InitTss[0].ist7 = ist_ptr;


    #ifdef APIC
        InitIoApic();
    #else
        Init8259a();
    #endif

    InitLocalApic();

    softirq_init();


    // *(unsigned char *)0xffff800000020000 = 0xf4; // hlt assistance processor

    scheduler_init();
    
    smp_init();

    // IPI INIT 
    icr_entry.vector = 0;
    icr_entry.DelivMode = 0b101;
    icr_entry.TarMode = 0;
    icr_entry.Trigger = 0;
    icr_entry.DelivStatus = 0;
    icr_entry.short_hand = 0b11;
    icr_entry.delivery_target.x2apic.target = 0;

    wrmsr(0x830, *(unsigned long*)&icr_entry);

    for(unsigned long ap_index = 1; ap_index < 4; ap_index++){


        memset(&InitTss[ap_index], 0, sizeof(struct TssStruct));
        set_tss_descriptor(10 + (ap_index * 2), (unsigned int *)&InitTss[ap_index]);

        _stack_start = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
        ((struct TaskStruct*)(_stack_start - STACK_SIZE))->cpu_id = ap_index;

        ist_ptr = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
        ((struct TaskStruct*)(ist_ptr - STACK_SIZE))->cpu_id = ap_index;

        SetTss( (unsigned int *)&InitTss[ap_index], _stack_start, _stack_start, _stack_start, ist_ptr, ist_ptr, ist_ptr,\
                ist_ptr, ist_ptr, ist_ptr, ist_ptr);

        // IPI START UP
        icr_entry.short_hand = 0b00;
        icr_entry.vector = 0x20;
        icr_entry.DelivMode = 0b110;
        icr_entry.delivery_target.x2apic.target = ap_index;

        wrmsr(0x830, *(unsigned long*)&icr_entry);
        // Send again for Safety
        wrmsr(0x830, *(unsigned long*)&icr_entry);

    }

    // int x = 1/ 0;
    TaskInit();
    hpet_init();
    time_init();

    KeyboardInit();
    FloppyInit();

    // icr_entry.vector = 0xc8;
    // icr_entry.delivery_target.x2apic.target = 1;
    // icr_entry.DelivMode = 0x0;
    // wrmsr(0x830, *(unsigned long*)&icr_entry);

    // icr_entry.vector = 0xc9;
    // wrmsr(0x830, *(unsigned long*)&icr_entry);


    while (1){
        AnalyzeKeyCode();
    }

}