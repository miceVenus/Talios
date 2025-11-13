#include "cpu.h"
#include "printk.h"
#include "lib.h"
void CPUID(     unsigned int mop, unsigned int sop, unsigned int* eax, 
                unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

void CpuInit(){

    unsigned int eax, ebx, ecx, edx;
    char FatoryName[17] = {0};

    CPUID(0x0, 0x0, &eax, &ebx, &ecx, &edx);

    *(unsigned int *)&FatoryName[0] = ebx;
    *(unsigned int *)&FatoryName[4] = ecx;
    *(unsigned int *)&FatoryName[8] = edx;

    ColorPrintfk(BLUE, BLACK, "%s\n", FatoryName);

    CPUID(0x1, 0x0, &eax, &ebx, &ecx, &edx);

    ColorPrintfk(   BLUE, BLACK, "Family ID: %x Extend Family ID: %x, Model ID: %x, Extend Model ID: %x Processor Type: %x Stepping ID: %x\n", 
                    GetBits(eax, 8, 4), GetBits(eax, 20, 8), GetBits(eax, 4, 4), GetBits(eax, 15, 5), 
                    GetBits(eax, 12, 2), GetBits(eax, 0, 4));

    ColorPrintfk(   BLUE, BLACK, "Brand Index: %x CLFLUSH line size(bytes): %x, Maximum number of addressable IDs for logical processors in this physical package: %x, Initial APIC ID: %x \n", 
                    GetBits(ebx, 0, 8), GetBits(ebx, 8, 8) << 3, GetBits(ebx, 16, 8), GetBits(ebx, 24, 8));
    

    CPUID(0x80000000, 0x0, &eax, &ebx, &ecx, &edx);

    ColorPrintfk(BLUE, BLACK, "Maximum Input Value for Extended Function CPUID: %x\n", eax);

    CPUID(0x80000001, 0x0, &eax, &ebx, &ecx, &edx);

    ColorPrintfk(BLUE, BLACK, "Processor Signature and Feature Bits: %x\n", eax);
    ColorPrintfk(BLUE, BLACK, "Support LAHF/SAHF:%d \t Support LZCNT:%d \t Support PREFETCHW:%d \n", 
                GetBits(ecx, 0, 1), GetBits(ecx, 5, 1), GetBits(ecx, 8, 1));
    ColorPrintfk(BLUE, BLACK, "Support SYSCALL/SYSRET : %d \t Support Execute Disable Bit : %d \t Support 1GB-Pages:%d \n", 
                GetBits(edx, 11, 1), GetBits(edx, 20, 1), GetBits(edx, 26, 1));
    ColorPrintfk(BLUE, BLACK, "Support RDTSCP and IA32_TSC_AUX:%d \t Support  Intel 64 Architecture: %d \n", 
                GetBits(edx, 27, 1), GetBits(edx, 29, 1));
    
    for(unsigned int i = 0x80000002; i < 0x80000005; i++){
        CPUID(i, 0x0, &eax, &ebx, &ecx, &edx);
        *(unsigned int *)&FatoryName[0]     = eax;
        *(unsigned int *)&FatoryName[4]     = ebx;
        *(unsigned int *)&FatoryName[8]     = ecx;
        *(unsigned int *)&FatoryName[12]    = edx;

        ColorPrintfk(BLUE, BLACK, "%s ", FatoryName);
    }
    ColorPrintfk(BLUE, BLACK, "\n");

    CPUID(0x80000008, 0x0, &eax, &ebx, &ecx, &edx);

    ColorPrintfk(BLUE, BLACK, "Physical/Linear Address size %x Physical/Linear Address Bits: %x/%x\n", 
                GetBits(eax, 0, 8), GetBits(eax, 8, 8), GetBits(eax, 16, 16));
    ColorPrintfk(BLUE, BLACK, "Support WBNOINVD:%d \n", GetBits(ebx, 8, 1));

}