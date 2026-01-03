#include "smp.h"
#include "cpu.h"
#include "printk.h"
#include "lib.h"

void smp_init(){
    unsigned int eax, ecx, ebx, edx;

    // Get Tapology Structure With Asmblycode CPUID

    for(int i = 0; ; i++){
        CPUID(0xb, i, &eax, &ebx, &ecx, &edx);

        // eax[4：0] Bit Width
        // ebx[15:0] logical Processor In Current Level
        // ecx[15:8] 0 : End Tapology; 1 : SMT ; 3~255 : Core
        // ecx[7:0]  Max Tapology Level
        // edx       x2APIC ID In This Logical Processor
        if(!GetBits(ecx, 8, 8)) break;

        ColorPrintfk(   BLUE, BLACK, "Local APIC ID Package_../Core_2/SMT_1, type(%x) \n Width:%x, num of logical processor:%x\n", 
                        GetBits(ecx, 8, 8), GetBits(eax, 0, 5), GetBits(ebx, 0, 8));
    }
    ColorPrintfk(   BLUE, BLACK, "x2APIC ID Level:(%x) \t x2APIC ID :%x\n", GetBits(ecx, 0, 8), edx);

}