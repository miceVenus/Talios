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
#include "mmap.h"
#include "test/memory_test.h"
#include "disk.h"


void BRK(){

}

extern unsigned long _stack_start;
// extern unsigned int TssTable[];

extern struct GlobalMemManager MMS;
extern Time global_time;
extern struct TssStruct InitTss[NR_CPUS];

void main(){

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

    InitPageTable();
    vm_init();

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


    InitLocalApic();
    #ifdef APIC
        InitIoApic();
    #else
        Init8259a();
    #endif

    softirq_init();


    scheduler_init();
    
    KeyboardInit();
    FloppyInit();
    disk_init();

    TaskInit();
    time_init();
    hpet_init();
    
    smp_init();
    KernelThread(init, 10, 0);
    BRK();
    sched_idle();
}
