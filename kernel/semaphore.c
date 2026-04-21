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

void __acquire(semaphore_t *semaphore){
    wait_queue_t wait;
    ListInit(&wait.wait_list);
    wait.tsk = CURRENT;
    CURRENT->state = TASK_UNINTERRUPTABLE;
    ListForeAdd(&wait.wait_list, &semaphore->wait.wait_list);
    schedule();
    ListDelete(&wait.wait_list);
}

void semaphore_acquire(semaphore_t *semaphore){
    if(atomic_read(&semaphore->counter) > 0)
        atomic_dec(&semaphore->counter);
    else
        __acquire(semaphore);
}


void __release(semaphore_t *semaphore){
    wait_queue_t * wait = ContainerOf(&semaphore->wait.wait_list, wait_queue_t, wait_list);
    ListDelete(&wait->wait_list);
    wait->tsk->state = TASK_RUNING;
    insert_task_queue(wait->tsk);
}

void semaphore_release(semaphore_t *semaphore){
    if(ListIsEmpty(&semaphore->wait.wait_list))
        atomic_inc(&semaphore->counter);
    else
        __release(semaphore);
}

void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk){
    ListInit(&wait_queue->wait_list);
    wait_queue->tsk = tsk;
}

void semaphore_init(semaphore_t *semaphore, unsigned long count){
    atomic_write(&semaphore->counter, count);
    wait_queue_init(&semaphore->wait, NULL);
}
