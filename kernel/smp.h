#ifndef SMP_H
#define SMP_H

#include "task.h"
#define smp_cpu_id() (CURRENT->cpu_id)
extern unsigned int smp_apic_ids[NR_CPUS], smp_cpu_count;
void smp_discover(unsigned int bsp);
void smp_init(void);
void start_smp(unsigned int cpu);
void smp_kick(unsigned long mask);
void smp_timer_tick(void);
void vm_switch_root(int cpu, unsigned long pgd);
void smp_invalidate(unsigned long pgd, unsigned long address);
#endif
