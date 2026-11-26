#ifndef WAITQUEUE_H
#define WAITQUEUE_H

#include "lib.h"
#include "spin_lock.h"

typedef struct wait_queue_t{
    struct List wait_list;
    struct TaskStruct *tsk;
}wait_queue_t;


void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk);
void wake_up(wait_queue_t * wait_queue);
void sleep_on(wait_queue_t *queue, SpinLock_T *condition_lock);


#endif