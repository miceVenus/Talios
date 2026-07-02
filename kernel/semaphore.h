#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "atomic.h"
#include "lib.h"
#include "waitqueue.h"

struct TaskStruct;

typedef struct semaphore_t{
    atomic_t counter;
    wait_queue_t wait;
}semaphore_t;

void semaphore_init(semaphore_t *semaphore, unsigned long count);
void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk);

#endif