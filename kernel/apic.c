#include "apic.h"
#include "lib.h"
#include "cpu.h"
#include "printk.h"
#include "interrupt.h"
#include "gate.h"
#include "pci.h"
#include "memory.h"

// corrected machine check error interrupt
#define LAPIC_CMCI_REGISTER 0x82f

#define LAPIC_TIMER_REGISTER 0x832
#define LAPIC_THERMAL_MONITOR_REGISTER 0x833
#define LAPIC_PERF_COUNTER_REGISTER 0x834
#define LAPIC_LINT0_REGISTER 0x835
#define LAPIC_LINT1_REGISTER 0x836
#define LAPIC_ERROR_REGISTER 0x837

#define L_APIC_BASE_EN_BIT (1UL << 10)
#define L_APIC_BASE_EXTD_BIT (1UL << 11)


extern interrupt_t interrupt[24];
extern struct GlobalMemManager MMS;


struct IoApicMap IoApicMap;

void CPUID(  unsigned int mop, unsigned int sop, unsigned int* eax, 
        unsigned int* ebx, unsigned int* ecx, unsigned int* edx);

void enable_lapic(){
    unsigned long lapic_base = rdmsr(0x1b);
    lapic_base |= L_APIC_BASE_EN_BIT | L_APIC_BASE_EXTD_BIT;
    wrmsr(0x1b, lapic_base);
}

unsigned long get_lapic_id(){
    return rdmsr(0x802);
}

unsigned long get_lapic_version(){
    return rdmsr(0x803);
}

void init_lapic_svr(){
    // close EOI broadcast In bochs
    wrmsr(0x80f, 0x1ff);
}

void set_lapic_tpr(unsigned long priority){
    wrmsr(0x808, priority);
}

void set_lapic_lvt(unsigned long entry,unsigned long content){
    wrmsr(entry, content);
}

void mask_lapic_lvt(unsigned long entry){
    wrmsr(entry, 0x10000);
}

int check_apic_x2apic(){
    unsigned int eax, ebx, ecx, edx;
    CPUID(1, 0, &eax, &ebx, &ecx, &edx);
    return (GetBits(edx, 9, 1) & GetBits(ecx, 21, 1));
}


void InitLocalApic(){

    // unsigned int x, y;
    // unsigned short HasApic;
    // unsigned short Hasx2Apic;

    // HasApic     = GetBits(edx, 9, 1);
    // Hasx2Apic   = GetBits(ecx, 21, 1);

    // ColorPrintfk(BLUE, BLACK, "Has Apic : %d, Has x2 Apic : %d\n", HasApic, Hasx2Apic);
    

    if(!check_apic_x2apic()){
        ColorPrintfk(BLUE, BLACK, "This chip is not support for apic\n");
        return;
    }

    // INIT APIC BASE

    enable_lapic();
    init_lapic_svr();

    // __asm__ volatile(   "movq   $0x1b,  %%rcx   \n\t"
    //                     "rdmsr                  \n\t"
    //                     "bts    $10,    %%rax   \n\t"
    //                     "bts    $11,    %%rax   \n\t"
    //                     "wrmsr                  \n\t"
    //                     "movq   $0x1b,  %%rcx   \n\t"
    //                     "rdmsr                  \n\t":
    //                     "=a"(x), "=d"(y)::"memory");
    
    // ColorPrintfk(BLUE, BLACK, "IA 32 APIC BASE : %X\n", (((unsigned long)y << 32) + x));

    // // SHOW APIC ID
    // __asm__ volatile(   "movq   $0x802,  %%rcx  \n\t"
    //                     "rdmsr                  \n\t":
    //                     "=a"(x), "=d"(y)::"memory");
    ColorPrintfk(BLUE, BLACK, "LOCAL APIC ID : %X\n", get_lapic_id());

    // // SHOW APIC VERSION

    // __asm__ volatile(   "movq   $0x803,  %%rcx  \n\t"
    //                     "rdmsr                  \n\t":
    //                     "=a"(x), "=d"(y)::"memory");

    ColorPrintfk(BLUE, BLACK, "LOCAL APIC VERSION : %X\n", get_lapic_version());

    // MASK LVT

    mask_lapic_lvt(LAPIC_CMCI_REGISTER);
    mask_lapic_lvt(LAPIC_TIMER_REGISTER);
    mask_lapic_lvt(LAPIC_THERMAL_MONITOR_REGISTER);
    mask_lapic_lvt(LAPIC_PERF_COUNTER_REGISTER);
    mask_lapic_lvt(LAPIC_LINT0_REGISTER);
    mask_lapic_lvt(LAPIC_LINT1_REGISTER);
    mask_lapic_lvt(LAPIC_ERROR_REGISTER);

    // // TPR
    // __asm__ volatile(   "movq   $0x808,  %%rcx  \n\t"
    //                     "rdmsr                  \n\t":
    //                     "=a"(x), "=d"(y)::"memory");
    // ColorPrintfk(BLUE, BLACK, "TPR : %D\n", (((unsigned long)y << 32) + x));

    // // PPR
    // __asm__ volatile(   "movq   $0x80a,  %%rcx  \n\t"
    //                     "rdmsr                  \n\t":
    //                     "=a"(x), "=d"(y)::"memory");
    // ColorPrintfk(BLUE, BLACK, "PPR : %D\n", (((unsigned long)y << 32) + x));
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
    IoApicMap.VirtualDataAddr   =   (unsigned int *)(IoApicAddr + 0x10);
    unsigned long *tmp;

    unsigned long Cr3 = GetCr3();
    tmp =   (unsigned long *)((unsigned long)PHY_TO_VIRT(Cr3 & (~0xfff))) + GetBits(IoApicAddr, PAGE_GDT_SHIFT, 9);

    if(*tmp == 0){
        void *virtual = kmalloc(PAGE_4K_SIZE, 0);
        SetPDPT(tmp, VIRT_TO_PHY(virtual), 0x3);
    }
    tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + GetBits(IoApicAddr, PAGE_1G_SHIFT, 9);
    
    if(*tmp == 0){
        void *virtual = kmalloc(PAGE_4K_SIZE, 0);
        SetPD(tmp, VIRT_TO_PHY(virtual), 0x3);
    }

    tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + GetBits(IoApicAddr, PAGE_2M_SHIFT, 9);

    SetPDE(tmp, IoApicMap.PhysicalAddr, 0x83 | 8 | 16);

    FlushTLB();

    *IoApicMap.VirtualIndexAddr =   1;
    mfence();
    unsigned int version  = *IoApicMap.VirtualDataAddr;
    mfence();

    if(GetBits(version, 0, 8) == I440FX) IoApicMap.VirtualEoiAddr = NULL;
    else IoApicMap.VirtualEoiAddr = (unsigned int *)(IoApicAddr + 40);
}

void InitIoApic(){

    IoApicPageTableRemap();

    *IoApicMap.VirtualIndexAddr = 0;
    mfence();
    *IoApicMap.VirtualDataAddr = 0x0f000000;
    mfence();
    ColorPrintfk(BLUE, BLACK, "IO APIC ID %X", GetBits(*IoApicMap.VirtualDataAddr, 24, 4));
    mfence();

    unsigned int IoApicVersion;
    *IoApicMap.VirtualIndexAddr = 1;
    mfence();
    IoApicVersion = *IoApicMap.VirtualDataAddr;
    ColorPrintfk(BLUE, BLACK, "IO APIC VERSION : %X", GetBits(IoApicVersion, 0, 8));
    mfence();

    for(unsigned int i = 0x10; i < 0x40; i += 2){
        IoApicRteWrite(i, 0x10020 + ((i - 0x10) >> 1));
    }

    for(unsigned int i = 32; i < (32 + 24); i++){
        SetIntrGate(i, 0, interrupt[i-32]); // we have softirq so.. it is possible that stack space could be overwrited when we set ist
    }

    OUT8b(0x21, 0xff);
    OUT8b(0xa1, 0xff);

    InitLocalApic();

    if(GetBits(IoApicVersion, 0, 8) == I440FX){
        // Enable intel 440 FX APIC
        unsigned int XBCS = ReadPci32(0, 1, 0, 0x4c);
        XBCS |= (1 << 24);
        WritePci32(0, 1, 0, 0x4c, XBCS);
    }else{
        // Enable QM intel APIC
        unsigned int RCBA   = ReadPci32(0, 31, 0, 0xF0);
        RCBA = RCBA & 0xffffc000;
        unsigned short OIC    = *(unsigned short*)PHY_TO_VIRT(RCBA + 0x31FE);
        mfence();
        *(unsigned short*)PHY_TO_VIRT(RCBA + 0x31FE) = OIC | 0x100;
        mfence();
    }

    sti();
}

void ApicEnable(unsigned long irq){
    unsigned long index = (irq << 1)- 0x30;
    unsigned long value = IoApicRteRead(index);
    IoApicRteWrite(index, value & (~0x10000));
}

void ApicAck(unsigned long irq){
    wrmsr(EOIR_MSR, 0x0);
}

void ApicInstall(unsigned long irq, void * arg){
    unsigned long value = (*(unsigned long*)arg);
    unsigned long index = (irq << 1)- 0x30;
    IoApicRteWrite(index, value);
}

void ApicUninstall(unsigned long irq){
    unsigned long index = (irq << 1)- 0x30;
    IoApicRteWrite(index, 0x10000);
}
void ApicDisable(unsigned long irq){
    unsigned long index = (irq << 1)- 0x30;
    unsigned long value = IoApicRteRead((irq << 1)- 0x30);
    IoApicRteWrite(index, value | 0x10000);
}
