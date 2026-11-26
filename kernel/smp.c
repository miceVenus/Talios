#include "smp.h"
#include "apic.h"
#include "gate.h"
#include "interrupt.h"
#include "schedule.h"
#include "hpet.h"
#include "mmap.h"
#include "printk.h"

#define IPI_TICK  0xc8
#define IPI_WAKE  0xc9
#define IPI_TLB   0xca
#define AP_BOOT_PHYS 0x20000UL

extern unsigned char _APU_boot_start[], _APU_boot_end[];
extern struct LocalMemManager InitLmm;
extern struct TssStruct InitTss[NR_CPUS];
extern union TaskUnion *InitTaskUnions[NR_CPUS];

unsigned int smp_apic_ids[NR_CPUS], smp_cpu_count = 1;
unsigned long smp_stack_tops[NR_CPUS];
static unsigned long online = 1;
static unsigned long active_root[NR_CPUS];

/* Page-table edits are serialized by the VM mutex. Acknowledgements are
 * sent before an old frame or a page-table page may be reused. */
static unsigned long flush_root, flush_addr, flush_pending;

static void send_mask(unsigned int vector, unsigned long mask){
    while(mask){
        int cpu = __builtin_ctzl(mask);
        wrmsr(0x830, ((unsigned long)smp_apic_ids[cpu] << 32) | vector);
        mask &= mask - 1;
    }
}
void smp_kick(unsigned long mask){
    send_mask(IPI_WAKE, mask & __atomic_load_n(&online, __ATOMIC_ACQUIRE) & ~(1UL << smp_cpu_id()));
}
void smp_timer_tick(void){
    send_mask(IPI_TICK, __atomic_load_n(&online, __ATOMIC_ACQUIRE) & ~1UL);
}
static void tick_ipi(struct PtRegs *regs, unsigned long nr, unsigned long arg){
    (void)regs; (void)nr; (void)arg;
    scheduler_tick();
}
static void wake_ipi(struct PtRegs *regs, unsigned long nr, unsigned long arg){
    (void)regs; (void)nr; (void)arg;
    CURRENT->flags |= NEED_SCHEDULE;
}
static void tlb_ipi(struct PtRegs *regs, unsigned long nr, unsigned long arg){
    (void)regs; (void)nr; (void)arg;
    unsigned long bit = 1UL << smp_cpu_id();
    if(!(__atomic_load_n(&flush_pending, __ATOMIC_ACQUIRE) & bit)) return;
    int active = GetCr3() == flush_root;
    if(active) __asm__ volatile("invlpg (%0)" :: "r"(flush_addr) : "memory");
    __atomic_fetch_and(&flush_pending, ~bit, __ATOMIC_RELEASE);
#ifdef TALIOS_SCHED_CHECK
    static int reported;
    if(active && !__atomic_exchange_n(&reported, 1, __ATOMIC_RELAXED))
        ColorPrintfk(BLUE, BLACK, "SCHED remote TLB invalidation\n");
#endif
}

void vm_switch_root(int cpu, unsigned long pgd){
    /* Interrupts are disabled across publication and the flushing CR3 load. */
    __atomic_store_n(&active_root[cpu], pgd, __ATOMIC_SEQ_CST);
    SetCr3(pgd);
}

void smp_invalidate(unsigned long pgd, unsigned long address){
    unsigned long irq = irq_save();
    unsigned long others = __atomic_load_n(&online, __ATOMIC_ACQUIRE) & ~(1UL << smp_cpu_id());
    if(GetCr3() == pgd) __asm__ volatile("invlpg (%0)" :: "r"(address) : "memory");
    if(others){
        unsigned long targets = 0;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        while(others){
            int cpu = __builtin_ctzl(others);
            if(__atomic_load_n(&active_root[cpu], __ATOMIC_SEQ_CST) == pgd) targets |= 1UL << cpu;
            others &= others - 1;
        }
        if(targets){
            flush_root = pgd;
            flush_addr = address;
            __atomic_store_n(&flush_pending, targets, __ATOMIC_RELEASE);
            send_mask(IPI_TLB, targets);
            while(__atomic_load_n(&flush_pending, __ATOMIC_ACQUIRE)) __asm__ volatile("pause");
        }
    }
    irq_restore(irq);
}

void smp_init(void){
    for(int v = IPI_TICK; v <= IPI_TLB; v++) SetIntrGate(v, 0, smp_interrupt[v - 0xc8]);
    regitser_ipi(IPI_TICK, NULL, tick_ipi, 0, NULL, "scheduler tick");
    regitser_ipi(IPI_WAKE, NULL, wake_ipi, 0, NULL, "reschedule");
    regitser_ipi(IPI_TLB, NULL, tlb_ipi, 0, NULL, "TLB invalidation");
    active_root[0] = GetCr3();
    smp_discover(get_lapic_id());
    memcopy(_APU_boot_start, PHY_TO_VIRT(AP_BOOT_PHYS), _APU_boot_end - _APU_boot_start);
    for(unsigned int cpu = 1; cpu < smp_cpu_count; cpu++){
        union TaskUnion *idle = kmalloc(STACK_SIZE, 0);
        void *exception_stack = kmalloc(STACK_SIZE, 0);
        if(!idle || !exception_stack){
            if(idle) kfree(idle);
            if(exception_stack) kfree(exception_stack);
            continue;
        }
        memset(idle, 0, sizeof(TaskStruct) + sizeof(struct ThreadStruct));
        InitTaskUnions[cpu] = idle;
        TaskStruct *t = &idle->task;
        t->thread = (struct ThreadStruct *)(t + 1);
        t->cpu_id = cpu;
        t->state = TASK_RUNNING;
        t->flags = PF_KTHREAD;
        t->lmm = &InitLmm;
        t->parent = &InitTaskUnions[0]->task;
        t->AddrLimit = ~0UL;
        ListInit(&t->list);
        unsigned long top = (unsigned long)idle + STACK_SIZE;
        t->thread->rsp0 = smp_stack_tops[cpu] = top;
        t->thread->fs = t->thread->gs = KERNEL_DS;
        unsigned long ist = (unsigned long)exception_stack + STACK_SIZE;
        ((TaskStruct *)exception_stack)->cpu_id = cpu;
        SetTss((unsigned int *)&InitTss[cpu], top, top, top, ist, ist, ist, ist, ist, ist, ist);
        set_tss_descriptor(10 + 2 * cpu, &InitTss[cpu]);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        unsigned long target = (unsigned long)smp_apic_ids[cpu] << 32;
        wrmsr(0x830, target | 0xc500);  /* INIT, level assert */
        hpet_delay_us(10000);
        for(int attempt = 0; attempt < 2; attempt++){
            wrmsr(0x830, target | 0x600 | (AP_BOOT_PHYS >> 12));
            hpet_delay_us(200);
            if(__atomic_load_n(&online, __ATOMIC_ACQUIRE) & (1UL << cpu)) break;
        }
        for(int n = 0; n < 1000 && !(__atomic_load_n(&online, __ATOMIC_ACQUIRE) & (1UL << cpu)); n++)
            hpet_delay_us(100);
        if(!(__atomic_load_n(&online, __ATOMIC_ACQUIRE) & (1UL << cpu)))
            ColorPrintfk(RED, BLACK, "CPU %d startup timed out\n", cpu);
    }
    unsigned long mask = __atomic_load_n(&online, __ATOMIC_ACQUIRE);
    unsigned int n = 0;
    while(mask){ n++; mask &= mask - 1; }
    ColorPrintfk(BLUE, BLACK, "SMP online %d/%d CPUs\n", n, smp_cpu_count);
}

void start_smp(unsigned int cpu){
    enable_lapic();
    init_lapic_svr();
    set_lapic_tpr(0);
    LTR(10 + 2 * cpu);
    vm_cpu_init();
    vm_switch_root(cpu, (unsigned long)InitLmm.pgd);
    task_cpu_init();
    __atomic_fetch_or(&online, 1UL << cpu, __ATOMIC_RELEASE);
    sched_idle();
}
