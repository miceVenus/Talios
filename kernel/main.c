#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "lib.h"

extern struct GlobalMemDescriptor MMS;
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

    for(int i = 0; i < 64; i++){
        struct Page *p = AllocPage(ZONE_NORMAL_INDEX, 64, PG_Active);
        if(p != NULL)
        ColorPrintfk(BLUE, BLACK, "Alloc 64 Pages, In %p, BitMap1 %X, BitMap2 %X\n", p->PhyAddr, MMS.BitsMap[0], MMS.BitsMap[1]);
        else
        break;
    }

    int i = 1/0;
    while (1) {}
}