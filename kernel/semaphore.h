#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "atomic.h"
#include "lib.h"
#include "task.h"
#include "schedule.h"

typedef struct wait_queue_t{
    struct List wait_list;
    struct TaskStruct *tsk;
}wait_queue_t;

typedef struct semaphore_t{
    atomic_t counter;
    wait_queue_t wait;
}semaphore_t;

void semaphore_init(semaphore_t *semaphore, unsigned long count);

#endif