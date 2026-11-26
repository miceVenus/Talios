#include "semaphore.h"
#include "task.h"
#include "schedule.h"

void semaphore_init(semaphore_t *sem, unsigned long count){
    spin_lock_init(&sem->lock);
    sem->count = count;
    wait_queue_init(&sem->waiters, NULL);
}

void semaphore_acquire(semaphore_t *sem){
    unsigned long irq = irq_save();
    spin_lock(&sem->lock);
    if(sem->count){
        sem->count--;
        spin_unlock(&sem->lock);
    }else{
        wait_queue_t waiter;
        wait_queue_init(&waiter, CURRENT);
        ListForeAdd(&waiter.wait_list, &sem->waiters.wait_list);
        sched_prepare_sleep();
        spin_unlock(&sem->lock);
        schedule();
    }
    irq_restore(irq);
}

void semaphore_release(semaphore_t *sem){
    unsigned long irq = irq_save();
    spin_lock(&sem->lock);
    if(ListIsEmpty(&sem->waiters.wait_list))
        sem->count++;
    else
        wake_up(&sem->waiters);
    spin_unlock(&sem->lock);
    irq_restore(irq);
}
