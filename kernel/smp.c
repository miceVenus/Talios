#include "smp.h"
#include "cpu.h"
#include "apic.h"
#include "printk.h"
#include "lib.h"
#include "gate.h"
#include "spin_lock.h"
#include "interrupt.h"
#include "schedule.h"

extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];
extern struct LocalMemManager InitLmm;
extern unsigned long _stack_start;
extern struct TaskStruct* InitTask[NR_CPUS];
extern union TaskUnion* InitTaskUnions[NR_CPUS];
extern struct ThreadStruct* InitThreads[NR_CPUS];

enum ICR_IDENTIFIER{
    ICR_NO_SHORTHAND    = 0x00,
    ICR_SELF_SHORTHAND  = 1,
    ICR_ALL_SHORTHAND   = 2,
    ICR_EXCLUDE_ALL_SHORTHAND   = 3,
    ICR_EDGE_TRIGGE     = 0,
    ICR_LEVEL_TRIGGE    = 1,
    ICR_INVALID_LEVEL_TRIGGE  = 1,
    ICR_VALID_LEVEL_TRIGGE  = 1,
    ICR_FREE_DELIVERY       = 0,
    ICR_HANG_DELIVERY       = 1,
    ICR_PHYSIC_TARGET_MODE  = 0,
    ICR_LOGICAL_TARGET_MODE = 1,
    ICR_FIXED_DELIVERY_MODE = 0,
    ICR_LOWER_PRIORITY_DELIVERY_MODE,
    ICR_SMI_DELIVERY_MODE,
    ICR_NMI_DELIVERY_MODE,
    ICR_INIT_DELIVERY_MODE,
    ICR_START_UP_DELIVERY_MODE
};

void ipi_0x200(struct PtRegs *regs, unsigned long nr, unsigned long arg){

    // if(ContainerOf(ListNext(&timer_list_header.list), timer_list, list)->expire_jiffies <= jiffies)
    // add_softirq_status(TIME_SIRQ);

    struct TaskStruct * current = CURRENT;
    scheduler *task_scheduler = &task_schedulers[current->cpu_id];

    switch (current->priority){
        case 0:
        case 1:
            task_scheduler->CPU_exec_task_jiffies -= 1;
            current->vrun_time += 1;
            break;

        case 2:
            task_scheduler->CPU_exec_task_jiffies -= 2;
            current->vrun_time += 2;
            break;
    }

    if(task_schedulers[CURRENT->cpu_id].CPU_exec_task_jiffies <= 0){
            CURRENT->flags |= NEED_SCHEDULE;
    }
}

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
        SetIntrGate(i, 0, smp_interrupt[i - 200]);
    }
    memset(smp_ipi_desc, 0, sizeof(IrqDescT) * SMP_IPI_IRQS);

    regitser_ipi(0xc8, NULL, &ipi_0x200, NULL, NULL, "IPI 0x200");

    ColorPrintfk(   BLUE, BLACK, "x2APIC ID Level:(%x) \t x2APIC ID :%x\n", GetBits(ecx, 0, 8), edx);
    memcopy(_APU_boot_start, (void *)(0xffff800000020000), (unsigned long)_APU_boot_end - (unsigned long)_APU_boot_start);
}


void interrupt_cpu(unsigned long nr, unsigned long cpu){
    int all = cpu == (unsigned long)-1 ? 1 : 0;

    IcrEntry icr_entry;
    icr_entry.vector = nr;
    icr_entry.delivery_target.x2apic.target = all ? 0 : cpu;
    icr_entry.Trigger = ICR_EDGE_TRIGGE;
    icr_entry.DelivMode = ICR_FIXED_DELIVERY_MODE;
    icr_entry.DelivStatus = ICR_FREE_DELIVERY;
    icr_entry.DriveLevel = ICR_INVALID_LEVEL_TRIGGE;
    icr_entry.short_hand = all ?  ICR_EXCLUDE_ALL_SHORTHAND: ICR_NO_SHORTHAND;
    icr_entry.TarMode = ICR_PHYSIC_TARGET_MODE;
    wrmsr(MSR_ICR, *(unsigned long *)&icr_entry);
}


void start_smp(){

    if(!check_apic_x2apic()){
        ColorPrintfk(BLUE, BLACK, "This chip is not support for apic\n");
        hlt();
    }

    enable_lapic();
    init_lapic_svr();

    unsigned long ap_id = get_lapic_id();


    InitTaskUnions[ap_id] = CURRENT;

    CURRENT->state  =   TASK_UNINTERRUPTABLE;
    CURRENT->flags  =   PF_KTHREAD;
    CURRENT->lmm    =   &InitLmm;             
    CURRENT->thread =   (struct ThreadStruct*)(CURRENT + 1);

    memset((void *)CURRENT->thread, 0, sizeof(struct ThreadStruct));

    CURRENT->thread->rsp0 = _stack_start;
    CURRENT->thread->rsp  = _stack_start;
    CURRENT->thread->fs     =   KERNEL_DS,
    CURRENT->thread->gs     =   KERNEL_DS,

    InitThreads[ap_id]      =   CURRENT->thread;

    CURRENT->AddrLimit  = 0xffff800000000000;   
    CURRENT->pid        = 0;   
    CURRENT->counter    = 1;                    
    CURRENT->signal     = 0;                    
    CURRENT->priority   = 2;                    
    CURRENT->vrun_time  = 0;                    
    CURRENT->preempt_count = 0;                 
    CURRENT->cpu_id     = ap_id;
    
    InitTask[ap_id] = CURRENT;

    // spin_lock(&smp_lock);
    LTR(10 + (ap_id * 2));
    // __asm__ volatile("xchg %bx, %bx");

    // int x = 1/0;

    sti();
    ColorPrintfk(BLUE, BLACK, "configuration finished in cpu : %X, rsp: %X\n", ap_id, _stack_start);
    TaskInit();

    __UNLOCK(&smp_lock);

    while (1){
        hlt();
    }
}
