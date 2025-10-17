#include <stdint.h>
#include <stddef.h>
#include "printk.h"
#include "gate.h"
#include "trap.h"

void main(){
    PrintkInit();
    
    // LTR(8);

    SetTss( 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,\
            0xffff800000007c00, 0xffff800000007c00);

    SysVectorInit();
    ColorPrintfk(WHITE, BLACK, "This is a Test Line\n");
    ColorPrintfk(WHITE, BLACK, "This is another Test Line %d \n", 1289);
    ColorPrintfk(WHITE, BLACK, "This is another Test Line %X \n", 123135);
    ColorPrintfk(WHITE, BLACK, "This is another Test Line %p \n", (void*)SetTss);

    int i = 1/0;
    while (1) {}
}