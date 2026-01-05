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

    IcrEntry icr_entry = {0};

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

    // IPI START UP

    icr_entry.vector = 0x20;
    icr_entry.DelivMode = 0b110;

    wrmsr(0x830, *(unsigned long*)&icr_entry);
    // Send again for Safety
    wrmsr(0x830, *(unsigned long*)&icr_entry);


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