#include "schedule.h"
#include "smp.h"
#ifdef TALIOS_SCHED_CHECK
#include "apic.h"
#include "printk.h"
#endif

extern union TaskUnion *InitTaskUnions[NR_CPUS];

/* The table lock pins its members and serializes selection of the minimum.
 * Always take it before a task lock. No device or VM lock is acquired here. */
static struct List tasks;
static SpinLock_T table_lock;
static long min_vrun_time, next_pid = 1;
static unsigned long idle_mask;
static long budget[NR_CPUS];

static long tick_cost(const TaskStruct *task){
    return task->priority == 2 ? 2 : 1;
}

static void update_min(void){
    long lowest = __LONG_MAX__;
    for(struct List *p = tasks.next; p != &tasks; p = p->next){
        TaskStruct *t = ContainerOf(p, TaskStruct, list);
        if(t->state == TASK_RUNNABLE || t->state == TASK_RUNNING)
            lowest = min(lowest, t->vrun_time);
    }
    if(lowest != __LONG_MAX__) min_vrun_time = max(min_vrun_time, lowest);
}

void scheduler_init(void){
    ListInit(&tasks);
}

void scheduler_tick(void){
    TaskStruct *t = CURRENT;
    int cpu = t->cpu_id;
    if(t == &InitTaskUnions[cpu]->task) return;
    unsigned long irq = spin_lock_irqsave(&table_lock);
    raw_spin_lock(&t->lock);
    long cost = tick_cost(t);
    t->vrun_time += cost;
    budget[cpu] -= cost;
    if(budget[cpu] <= 0) t->flags |= NEED_SCHEDULE;
    raw_spin_unlock(&t->lock);
    update_min();
    spin_unlock_irqrestore(&table_lock, irq);
}

/* The caller still holds the lock protecting its wait condition. A wake
 * before schedule() sees a CPU owner and restores RUNNING in place. */
void sched_prepare_sleep(void){
    TaskStruct *t = CURRENT;
    unsigned long irq = spin_lock_irqsave(&table_lock);
    raw_spin_lock(&t->lock);
    t->state = TASK_UNINTERRUPTABLE;
    raw_spin_unlock(&t->lock);
    spin_unlock_irqrestore(&table_lock, irq);
}

void wakeup_process(TaskStruct *t){
    unsigned long irq = spin_lock_irqsave(&table_lock);
    unsigned long kick = 0;
    raw_spin_lock(&t->lock);
    if(t->state == TASK_UNINTERRUPTABLE){
        t->vrun_time = max(t->vrun_time, min_vrun_time - tick_cost(t));
        t->state = t->cpu_id < 0 ? TASK_RUNNABLE : TASK_RUNNING;
        if(t->state == TASK_RUNNABLE){
            kick = idle_mask;
            CURRENT->flags |= NEED_SCHEDULE;
        }
    }
    raw_spin_unlock(&t->lock);
    raw_spin_unlock(&table_lock);
    smp_kick(kick);
    irq_restore(irq);
}

long sched_publish(TaskStruct *child){
    TaskStruct *parent = CURRENT;
    unsigned long irq = spin_lock_irqsave(&table_lock);
    if(child->vfork_parent){
        raw_spin_lock(&parent->lock);
        parent->state = TASK_UNINTERRUPTABLE;
        raw_spin_unlock(&parent->lock);
    }
    long pid = child->pid = next_pid++;
    child->cpu_id = -1;
    child->state = TASK_RUNNABLE;
    child->vrun_time = max(parent->vrun_time, min_vrun_time);
    ListForeAdd(&child->list, &tasks);
    unsigned long kick = idle_mask;
    int wait = child->vfork_parent != NULL;
    parent->flags |= NEED_SCHEDULE;
    raw_spin_unlock(&table_lock);
    smp_kick(kick);
    if(wait) schedule();
    irq_restore(irq);
    return pid;
}

void schedule(void){
    unsigned long irq = spin_lock_irqsave(&table_lock);
    TaskStruct *prev = CURRENT;
    int cpu = prev->cpu_id;
    TaskStruct *idle = &InitTaskUnions[cpu]->task;
    idle_mask &= ~(1UL << cpu);
    raw_spin_lock(&prev->lock);
    prev->flags &= ~NEED_SCHEDULE;
    if(prev->state == TASK_RUNNING) prev->state = TASK_RUNNABLE;

    TaskStruct *next = idle;
    long runnable = 0;
    for(struct List *p = tasks.next; p != &tasks; p = p->next){
        TaskStruct *t = ContainerOf(p, TaskStruct, list);
        if(t != prev) raw_spin_lock(&t->lock);
        if(t->state == TASK_RUNNABLE && (t == prev || t->cpu_id < 0)){
            runnable++;
            if(next == idle || t->vrun_time < next->vrun_time) next = t;
        }
        if(t != prev) raw_spin_unlock(&t->lock);
    }
    if(next != prev) raw_spin_lock(&next->lock);
    next->state = TASK_RUNNING;
    next->cpu_id = cpu;
    if(next != idle){
        /* Rotate ties without modifying the virtual time. */
        ListDelete(&next->list);
        ListForeAdd(&next->list, &tasks);
        long slice = max(1L, 4 / runnable);
        budget[cpu] = slice * (next->priority == 2 ? 3 : 1);
    }
    update_min();
    if(next == prev){
        raw_spin_unlock(&prev->lock);
        spin_unlock_irqrestore(&table_lock, irq);
        return;
    }

    raw_spin_unlock(&next->lock);
    raw_spin_unlock(&table_lock);
    vm_switch_root(cpu, (unsigned long)next->lmm->pgd);
    switch_context(prev, next, &prev->thread->rsp, &prev->thread->rip,
                   next->thread->rsp, next->thread->rip);
    irq_restore(irq);
}

#ifdef TALIOS_SCHED_CHECK
/* Independently record the stack each CPU actually switched onto. */
static int check_ownership(TaskStruct *task){
    static SpinLock_T lock;
    static TaskStruct *owners[NR_CPUS];
    static int reported;
    unsigned int cpu = 0, apic = get_lapic_id();
    while(cpu < smp_cpu_count && smp_apic_ids[cpu] != apic) cpu++;
    if(cpu == smp_cpu_count || task->cpu_id != (long)cpu) return -1;
    int busy = 0, result = 0;
    raw_spin_lock(&lock);
    owners[cpu] = (task->flags & PF_KTHREAD) ? NULL : task;
    for(unsigned int i = 0; i < smp_cpu_count; i++){
        if(owners[i]) busy++;
        if(i != cpu && owners[i] == task) result = -1;
    }
    if(!result && busy == (int)smp_cpu_count && !reported){
        result = busy;
        reported = 1;
    }
    raw_spin_unlock(&lock);
    return result;
}
#endif

/* Called on the incoming kernel stack, including the first dispatch of a
 * new task. Only now can another CPU use or free the outgoing stack. */
void sched_finish_switch(TaskStruct *prev){
    int dead = prev->state == TASK_DEAD;
#ifdef TALIOS_SCHED_CHECK
    int ownership = check_ownership(CURRENT);
#endif
    prev->cpu_id = -1;
    raw_spin_unlock(&prev->lock);
    if(dead) kfree(prev);
#ifdef TALIOS_SCHED_CHECK
    if(ownership < 0) ColorPrintfk(RED, BLACK, "SCHED FAIL task active on two CPUs\n");
    if(ownership > 0) ColorPrintfk(BLUE, BLACK, "SCHED concurrent tasks %d\n", ownership);
#endif
}

void sched_exit(void){
    cli();
    TaskStruct *t = CURRENT;
    raw_spin_lock(&table_lock);
    raw_spin_lock(&t->lock);
    ListDelete(&t->list);
    for(struct List *p = tasks.next; p != &tasks; p = p->next){
        TaskStruct *child = ContainerOf(p, TaskStruct, list);
        if(child->parent == t) child->parent = &InitTaskUnions[0]->task;
    }
    t->state = TASK_DEAD;
    raw_spin_unlock(&t->lock);
    raw_spin_unlock(&table_lock);
    schedule();
    __builtin_unreachable();
}

void sched_idle(void){
    int cpu = CURRENT->cpu_id;
    for(;;){
        cli();
        raw_spin_lock(&table_lock);
        struct List *p = tasks.next;
        while(p != &tasks && ContainerOf(p, TaskStruct, list)->state != TASK_RUNNABLE)
            p = p->next;
        int has_work = p != &tasks;
        if(!has_work) idle_mask |= 1UL << cpu;
        raw_spin_unlock(&table_lock);
        if(has_work) schedule();
        else __asm__ volatile("sti\n\thlt\n.globl sched_idle_resume\nsched_idle_resume:" ::: "memory");
    }
}
