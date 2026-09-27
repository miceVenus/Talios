#include "semaphore.h"
#include "task.h"
#include "schedule.h"


void atomic_write(atomic_t * atomic, long value);
long atomic_read(atomic_t * atomic);
void atomic_dec(atomic_t * atomic);
void atomic_inc(atomic_t * atomic);

void ListInit(struct List *list);
int ListIsEmpty(struct List *list);
void ListForeAdd(struct List *new, struct List *list);
int ListDelete(struct List *list);

void semaphore_init(semaphore_t *semaphore, unsigned long count){
    atomic_write(&semaphore->count, count);
    wait_queue_init(&semaphore->waiters, NULL);
}


void semaphore_acquire(struct semaphore *sem){

    spin_lock(&sem->lock);

    if (sem->count > 0) {
        sem->count--;
        spin_unlock(&sem->lock);
        return;
    }

    spin_unlock(&sem->lock);
    sleep_on(&sem->waiters);
}

void semaphore_release(struct semaphore *sem){

    spin_lock(&sem->lock);

    if (!ListIsEmpty(&sem->waiters.wait_list)) {
        wake_up(&sem->waiters, TASK_RUNING);
    } else {
        sem->count++;
    }

    spin_unlock(&sem->lock);
}
