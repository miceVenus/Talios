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

    while (1){
        AnalyzeKeyCode();
    }
    
    TaskInit();
}