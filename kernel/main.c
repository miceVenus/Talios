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
#include "test/memory_test.h"


void BRK(){

}

extern unsigned long _stack_start;
// extern unsigned int TssTable[];
extern struct GlobalMemManager MMS;
extern SpinLock_T smp_lock;
extern Time global_time;

unsigned int global_ap_index;

void main(){

    IcrEntry icr_entry = {0};

    PrintkInit();
    
    LTR(10);  // Check And Reloade TR

    SetTss( TssTable, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00);

    SysVectorInit();
        
    CpuInit();

    InitMemory();

    SlabCacheInit();

    // run_memory_tests();

    InitPageTable();

    #ifdef APIC
        InitIoApic();
    #else
        Init8259a();
    #endif

    InitLocalApic();
    softirq_init();


    // *(unsigned char *)0xffff800000020000 = 0xf4; // hlt assistance processor
    hpet_init();
    time_init();
    
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

    for(global_ap_index = 1; global_ap_index < 4; global_ap_index++){

        spin_lock(&smp_lock);

        _stack_start = (unsigned long)kmalloc(STACK_SIZE, 0) + STACK_SIZE;
        unsigned int * ap_tss = (unsigned int *)kmalloc(128, 0);
        set_tss_descriptor(10 + (global_ap_index * 2), ap_tss);
        SetTss( ap_tss, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start, _stack_start,\
                _stack_start, _stack_start, _stack_start, _stack_start);

        // IPI START UP
        icr_entry.short_hand = 0b00;
        icr_entry.vector = 0x20;
        icr_entry.DelivMode = 0b110;
        icr_entry.delivery_target.x2apic.target = global_ap_index;

        wrmsr(0x830, *(unsigned long*)&icr_entry);
        // Send again for Safety
        wrmsr(0x830, *(unsigned long*)&icr_entry);

        spin_lock(&smp_lock);
        spin_unlock(&smp_lock);
    }

    // int x = 1/ 0;
    KeyboardInit();
    FloppyInit();

    // unsigned char * buffer = kmalloc(sizeof(char) * 1024, 0);
    // memset(buffer, 0, 1024);
    // floppy_read_sector(1024, buffer);
    // for(int i = 0; i < 1024; i++)
    //     ColorPrintfk(BLUE, BLACK, "%d", buffer[0]);
    // ColorPrintfk(BLUE, BLACK, "\n1");
    // buffer[0] = 4;

    // floppy_write_sector(1024, buffer);
    // floppy_read_sector(1024, buffer);
    // for(int i = 0; i < 1024; i++)
    //     ColorPrintfk(BLUE, BLACK, "%d", buffer[0]);

    icr_entry.vector = 0xc8;
    icr_entry.delivery_target.x2apic.target = 1;
    icr_entry.DelivMode = 0x0;
    wrmsr(0x830, *(unsigned long*)&icr_entry);

    icr_entry.vector = 0xc9;
    wrmsr(0x830, *(unsigned long*)&icr_entry);

    // unsigned long *test = (unsigned long *)PHY_TO_VIRT(0xfed00000);
    // ColorPrintfk(RED, BLACK, "Test HPET: %X", *test);

    // unsigned long *hpet_reg = (unsigned long *)PHY_TO_VIRT(0xfed00000);
    // unsigned int period = (unsigned int)((*hpet_reg) >> 32); 

    // unsigned long *hpet_config = (unsigned long *)PHY_TO_VIRT(0xfed00000 + 0x10);
    // *hpet_config |= 0x1;

    // unsigned long *hpet_counter = (unsigned long *)PHY_TO_VIRT(0xfed00000 + 0xf0);
    // unsigned long now1 = *hpet_counter; // 得到当前的 64 位计数值

    // unsigned long now2 = *hpet_counter; // 得到当前的 64 位计数值

    // ColorPrintfk(RED, BLACK, "Time Now1 Time Now2: %X, %X\n", now1, now2);


    while (1){
        AnalyzeKeyCode();
    }
    
    TaskInit();
}