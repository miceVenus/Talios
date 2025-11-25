#include "apic.h"
#include "lib.h"
#include "cpu.h"
#include "printk.h"
#include "interrupt.h"
#include "gate.h"
#include "memory.h"

extern interrupt_t interrupt[24];
extern struct GlobalMemManager MMS;

struct IoApicMap IoApicMap;

void CPUID(  unsigned int mop, unsigned int sop, unsigned int* eax, 
        unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

void InitLocalApic(){
    unsigned int eax, ebx, ecx, edx;
    unsigned int x, y;
    unsigned short HasApic;
    unsigned short Hasx2Apic;
    CPUID(1, 0, &eax, &ebx, &ecx, &edx);

    HasApic     = GetBits(edx, 9, 1);
    Hasx2Apic   = GetBits(ecx, 21, 1);

    ColorPrintfk(BLUE, BLACK, "Has Apic : %d, Has x2 Apic : %d\n", HasApic, Hasx2Apic);
    
    if(!(HasApic & Hasx2Apic)) return;

    // INIT APIC BASE
    __asm__ volatile(   "movq   $0x1b,  %%rcx   \n\t"
                        "rdmsr                  \n\t"
                        "bts    $10,    %%rax   \n\t"
                        "bts    $11,    %%rax   \n\t"
                        "wrmsr                  \n\t"
                        "movq   $0x1b,  %%rcx   \n\t"
                        "rdmsr                  \n\t":
                        "=a"(x), "=d"(y)::"memory");
    
    ColorPrintfk(BLUE, BLACK, "IA 32 APIC BASE : %X\n", (((unsigned long)y << 32) + x));

    // SHOW APIC ID
    __asm__ volatile(   "movq   $0x802,  %%rcx  \n\t"
                        "rdmsr                  \n\t":
                        "=a"(x), "=d"(y)::"memory");
    ColorPrintfk(BLUE, BLACK, "LOCAL APIC ID : %D\n", (((unsigned long)y << 32) + x));

    // SHOW APIC VERSION
    __asm__ volatile(   "movq   $0x803,  %%rcx  \n\t"
                        "rdmsr                  \n\t":
                        "=a"(x), "=d"(y)::"memory");
    ColorPrintfk(BLUE, BLACK, "LOCAL APIC VERSION : %X\n", (((unsigned long)y << 32) + x));

    // MASK LVT

    __asm__ volatile(   "movq   $0x82F,  %%rcx  \n\t"     // CMCI
                        "wrmsr                  \n\t"
                        "movq   $0x832,  %%rcx  \n\t"     // TIMER
                        "wrmsr                  \n\t"
                        "movq   $0x833,  %%rcx  \n\t"     // THERMAL MONITOR
                        "wrmsr                  \n\t"
                        "movq   $0x834,  %%rcx  \n\t"     // PERF COUNTER
                        "wrmsr                  \n\t"
                        "movq   $0x835,  %%rcx  \n\t"     // LINT0
                        "wrmsr                  \n\t"
                        "movq   $0x836,  %%rcx  \n\t"     // LINT1
                        "wrmsr                  \n\t"
                        "movq   $0x837,  %%rcx  \n\t"     // ERROR
                        "wrmsr                  \n\t"::
                        "a"(0x10000), "d"(0x0):"memory");
    
    // TPR
    __asm__ volatile(   "movq   $0x808,  %%rcx  \n\t"
                        "rdmsr                  \n\t":
                        "=a"(x), "=d"(y)::"memory");
    ColorPrintfk(BLUE, BLACK, "TPR : %D\n", (((unsigned long)y << 32) + x));

    // PPR
    __asm__ volatile(   "movq   $0x80a,  %%rcx  \n\t"
                        "rdmsr                  \n\t":
                        "=a"(x), "=d"(y)::"memory");
    ColorPrintfk(BLUE, BLACK, "PPR : %D\n", (((unsigned long)y << 32) + x));
}

unsigned long IoApicRteRead(unsigned char index){
    unsigned long ret;
    *IoApicMap.VirtualIndexAddr = index + 1;
    mfence();

    ret = (unsigned long)*IoApicMap.VirtualDataAddr;
    ret <<= 32;
    mfence();
    
    *IoApicMap.VirtualIndexAddr = index;
    mfence();
    ret |= (unsigned long)*IoApicMap.VirtualDataAddr;

    mfence();

    return ret;
}

void IoApicRteWrite(unsigned char index, unsigned long value){
    *IoApicMap.VirtualIndexAddr = index;
    mfence();

    *IoApicMap.VirtualDataAddr = value & 0xffffffff;
    mfence();

    *IoApicMap.VirtualIndexAddr = index + 1;
    mfence();

    *IoApicMap.VirtualDataAddr = value >> 32;
    mfence();
}

void IoApicPageTableRemap(){
    unsigned long IoApicAddr    =   (unsigned long)PHY_TO_VIRT(0xfec00000);
    IoApicMap.PhysicalAddr      =   0xfec00000;
    IoApicMap.VirtualIndexAddr  =   (unsigned char *)IoApicAddr;
    IoApicMap.VirtualDataAddr   =   (unsigned int *)(IoApicAddr + 10);
    unsigned long *tmp;

    *IoApicMap.VirtualIndexAddr =   1;
    mfence();
    unsigned int version  = *IoApicMap.VirtualDataAddr;
    mfence();

    if(GetBits(version, 0, 8) == 0x11) IoApicMap.VirtualEoiAddr = NULL;
    else IoApicMap.VirtualEoiAddr = (unsigned int *)(IoApicAddr + 40);

    struct Page* page       = MMS.PagesGroup + PAGE_2M_INDEX(IoApicMap.PhysicalAddr);

    unsigned long Cr3 = GetCr3();
    tmp =   (unsigned long *)((unsigned long)PHY_TO_VIRT(Cr3 & (~0xfff))) + 
            GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_GDT_SHIFT, 9);

    if(*tmp == 0){
        void *virtual = kmalloc(PAGE_4K_SIZE, 0);
        SetPDPT(tmp, VIRT_TO_PHY(virtual), 0x3);
    }
    tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + 
            GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_1G_SHIFT, 9);
    
    if(*tmp == 0){
        void *virtual = kmalloc(PAGE_4K_SIZE, 0);
        SetPD(tmp, VIRT_TO_PHY(virtual), 0x3);
    }

    tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + 
            GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_2M_SHIFT, 9);
    SetPDE(tmp, page->PhyAddr, 0x83);

    FlushTLB();
}

void InitIoApic(){

    *IoApicMap.VirtualIndexAddr = 0;
    mfence();
    *IoApicMap.VirtualDataAddr = 0x0f000000;
    mfence();
    ColorPrintfk(BLUE, BLACK, "IO APIC ID %X", GetBits(*IoApicMap.VirtualDataAddr, 24, 4));
    mfence();

    *IoApicMap.VirtualIndexAddr = 1;
    mfence();
    ColorPrintfk(BLUE, BLACK, "IO APIC VERSION : %X", GetBits(*IoApicMap.VirtualDataAddr, 0, 8));
    mfence();

    for(unsigned int i = 0x10; i < 0x40; i += 2){
        IoApicRteWrite(i, 0x10020 + ((i - 0x10) >> 1));
    }

    for(unsigned int i = 32; i < (32 + 24); i++){
        SetIntrGate(i, 2, interrupt[i-32]);
    }

    IoApicRteWrite(0x12, 0x21);

    OUT8b(0x21, 0xff);
    OUT8b(0xa1, 0xff);

    InitLocalApic();
    IoApicPageTableRemap();

    sti();
}