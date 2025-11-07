#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "lib.h"
#include "interrupt.h"
#include "task.h"

extern struct GlobalMemManager MMS;
void BRKP(){

};
void main(){
    PrintkInit();
    
    LTR(8);  // Check And Reloade TR

    SetTss( 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00);

    SysVectorInit();
    InitMemory();

    InterruptInit();

    TaskInit();
}