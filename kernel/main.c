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
#include "test/memory_test.h"

void BRK(){

}
extern struct GlobalMemManager MMS;
void main(){
    PrintkInit();
    
    LTR(10);  // Check And Reloade TR

    SetTss( 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00);

    SysVectorInit();
        
    CpuInit();

    InitMemory();

    SlabCacheInit();

    // run_memory_tests();

    InitPageTable();

    // #ifdef APIC
    //     InitIoApic();
    // #else
    //     Init8259a();
    // #endif

    InitLocalApic();


    // *(unsigned char *)0xffff800000020000 = 0xf4; // hlt assistance processor

    // In order to start SMP Need To Init IPI

    /*

        ICR IN MSR 0x830

        delivery target In x2 APIC Is 63 ~ 32 In x APIC & APIC Is 63 ~ 56 

        ShortHand In x2 APIC xAPIC APIC Is bit 19 ~ 18
        00 No Short Hand
        01 Only Self
        10 Send To ALL (include self)
        11 Send To ALL (exclude self)

        Trigger Mode bit 15 0 means edge trigger 1 means level trigger
        Drive Level bit 14 0 means invalid 1 means valid
        Delivery Status bit 12 0 means free 1 means hangging
        Target Mode bit 11 0 means physic mode 1 means logical mode

        Deliver Mode 10~8 sames like APIC Delivery mode
        000 Fixed
        001 Lower Priority
        010 SMI
        100 NMI
        101 INIT
        110 Start UP

        Vector bit 7 ~ 0 means page frame number that AP start From
    
    */
    smp_init();

    // IPI INIT 

    wrmsr(0x830, 0xc4500);

    // IPI START UP
    wrmsr(0x830, 0xc4620);

    // Send again for Safety
    wrmsr(0x830, 0xc4620);

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


    while (1){
        AnalyzeKeyCode();
    }
    
    TaskInit();
}