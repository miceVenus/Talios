#include "softirq.h"
#include "lib.h"
#include "task.h"


static unsigned long pending[NR_CPUS];
static unsigned char servicing[NR_CPUS];

softirq softirq_vector[64] = {0};

void add_softirq_status(unsigned long status){
    unsigned long irq = irq_save();
    pending[CURRENT->cpu_id] |= status;
    irq_restore(irq);
}

unsigned long get_softirq_status(){
    int cpu = CURRENT->cpu_id;
    return servicing[cpu] ? 0 : pending[cpu];
}

void register_softirq(int nr, void (*action)(void * data), void *data){
    softirq_vector[nr].action = action;
    softirq_vector[nr].data = data;
}

void unregitser_softirq(int nr){
    softirq_vector[nr].action   = NULL;
    softirq_vector[nr].data     = NULL;
}

void softirq_init(){
    memset(pending, 0, sizeof(pending));
    memset(softirq_vector, 0, sizeof(softirq) * 64);
}

void do_softirq(){
    int cpu = CURRENT->cpu_id;
    unsigned long work = pending[cpu];
    pending[cpu] = 0;
    servicing[cpu] = 1;
    CURRENT->preempt_count++;
    sti();
    for(int i = 0; i < 64; i++)
        if((work & (1UL << i)) && softirq_vector[i].action)
            softirq_vector[i].action(softirq_vector[i].data);
    cli();
    CURRENT->preempt_count--;
    servicing[cpu] = 0;
}
