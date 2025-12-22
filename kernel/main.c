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

    #ifdef APIC
        InitIoApic();
    #else
        Init8259a();
    #endif

    KeyboardInit();
    FloppyInit();

    unsigned char * buffer = kmalloc(sizeof(char) * 1024, 0);
    memset(buffer, 0, 1024);
    floppy_read_sector(1024, buffer);
    for(int i = 0; i < 1024; i++)
        ColorPrintfk(BLUE, BLACK, "%d", buffer[0]);
    ColorPrintfk(BLUE, BLACK, "\n1");
    buffer[0] = 4;

    floppy_write_sector(1024, buffer);
    floppy_read_sector(1024, buffer);
    for(int i = 0; i < 1024; i++)
        ColorPrintfk(BLUE, BLACK, "%d", buffer[0]);


    while (1){
        AnalyzeKeyCode();
    }
    
    TaskInit();
}