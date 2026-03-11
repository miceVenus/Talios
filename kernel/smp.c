#include "smp.h"
#include "cpu.h"
#include "apic.h"
#include "printk.h"
#include "lib.h"
#include "gate.h"
#include "spin_lock.h"
#include "interrupt.h"

extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];

SpinLock_T smp_lock;

void smp_init(){
    spin_lock_init(&smp_lock);
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

    for(int i = 200; i < 210; i++){
        SetIntrGate(i, 2, smp_interrupt[i - 200]);
    }
    memset(smp_ipi_desc, 0, sizeof(IrqDescT) * SMP_IPI_IRQS);

    ColorPrintfk(   BLUE, BLACK, "x2APIC ID Level:(%x) \t x2APIC ID :%x\n", GetBits(ecx, 0, 8), edx);
    memcopy(_APU_boot_start, (void *)(0xffff800000020000), (unsigned long)_APU_boot_end - (unsigned long)_APU_boot_start);
}

void start_smp(){
    
    if(!check_apic_x2apic()){
        ColorPrintfk(BLUE, BLACK, "This chip is not support for apic\n");
        hlt();
    }

    enable_lapic();
    init_lapic_svr();


    ColorPrintfk(BLUE, BLACK, "configuration finished in cpu : %X\n", get_lapic_id());

    // spin_lock(&smp_lock);
    LTR(10 + (get_lapic_id() * 2));
    // __asm__ volatile("xchg %bx, %bx");

    // int x = 1/0;
    sti();

    while (1){
        hlt();
    }
}