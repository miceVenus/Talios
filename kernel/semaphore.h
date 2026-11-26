#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spin_lock.h"
#include "lib.h"
#include "waitqueue.h"

struct TaskStruct;

typedef struct semaphore {
    SpinLock_T lock;
    unsigned long count;
    wait_queue_t waiters;
}semaphore_t;

void semaphore_init(semaphore_t *semaphore, unsigned long count);
void semaphore_acquire(semaphore_t *sem);
void semaphore_release(semaphore_t *sem);

#endif